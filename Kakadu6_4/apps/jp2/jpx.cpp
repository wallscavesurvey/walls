/*****************************************************************************/
// File: jpx.cpp [scope = APPS/JP2]
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
   Implements the internal machinery whose external interfaces are defined
in the compressed-io header file, "jpx.h".
******************************************************************************/

#include <assert.h>
#include <string.h>
#include <math.h>
#include <kdu_utils.h>
#include "jpx.h"
#include "jpx_local.h"
#include "kdu_file_io.h"
#include "kdu_client_window.h"

/* Note Carefully:
      If you want to be able to use the "kdu_text_extractor" tool to
   extract text from calls to `kdu_error' and `kdu_warning' so that it
   can be separately registered (possibly in a variety of different
   languages), you should carefully preserve the form of the definitions
   below, starting from #ifdef KDU_CUSTOM_TEXT and extending to the
   definitions of KDU_WARNING_DEV and KDU_ERROR_DEV.  All of these
   definitions are expected by the current, reasonably inflexible
   implementation of "kdu_text_extractor".
      The only things you should change when these definitions are ported to
   different source files are the strings found inside the `kdu_error'
   and `kdu_warning' constructors.  These strings may be arbitrarily
   defined, as far as "kdu_text_extractor" is concerned, except that they
   must not occupy more than one line of text.
*/
#ifdef KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("E(jpx.cpp)",_id);
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("W(jpx.cpp)",_id);
#  define KDU_TXT(_string) "<#>" // Special replacement pattern
#else // !KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("Error in Kakadu File Format Support:\n");
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("Warning in Kakadu File Format Support:\n");
#  define KDU_TXT(_string) _string
#endif // !KDU_CUSTOM_TEXT

#define KDU_ERROR_DEV(_name,_id) KDU_ERROR(_name,_id)
 // Use the above version for errors which are of interest only to developers
#define KDU_WARNING_DEV(_name,_id) KDU_WARNING(_name,_id)
 // Use the above version for warnings which are of interest only to developers

#define JX_PI 3.141592653589793

/* ========================================================================= */
/*                            Internal Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                jx_check_metanode_before_add                        */
/*****************************************************************************/

static void jx_check_metanode_before_add_child(jx_metanode *state)
{
  if (state == NULL)
    { KDU_ERROR_DEV(e,0x02050901); e <<
      KDU_TXT("Trying to add new metadata to a `jpx_metanode' interface "
              "which is empty!");
    }
  if (state->manager->target != NULL)
    { KDU_ERROR_DEV(e,27); e <<
      KDU_TXT("Trying to add metadata to a `jpx_target' object "
              "whose `write_metadata' function has already been called.");
    }
  if ((state->rep_id == JX_CROSSREF_NODE) && (state->crossref != NULL) &&
      (state->crossref->link_type != JPX_METANODE_LINK_NONE) &&
      (state->crossref->link_type != JPX_GROUPING_LINK))
    { KDU_ERROR(e,0x02050902); e <<
      KDU_TXT("Trying to add descendants to a metadata node (`jpx_metanode') "
              "which is currently identified as a non-grouping link node.  "
              "Any link node (node represented by a cross-reference to "
              "another node in the metadata hierarchy) which has descendants "
              "must be a grouping link node -- one with link-type "
              "`JPX_GROUPING_LINK'.");
    }
}

/*****************************************************************************/
/* STATIC                        jx_midpoint                                 */
/*****************************************************************************/

static kdu_coords
  jx_midpoint(const kdu_coords &v1, const kdu_coords &v2)
{
  kdu_long val;
  kdu_coords result;
  val = v1.x; val += v2.x; result.x = (int)((val+1)>>1);
  val = v1.y; val += v2.y; result.y = (int)((val+1)>>1);
  return result;
}

/*****************************************************************************/
/* STATIC                    jx_project_to_line                              */
/*****************************************************************************/

static bool
  jx_project_to_line(kdu_coords v1, kdu_coords v2, kdu_coords &point)
  /* This function projects the `point', as found on entry, to the nearest
     point on the line which intersects locations `v1' and `v2', returning
     the projected point (rounded to the nearest integer coordinates) via
     `point'.  If `v1' and `v2' are identical, the function returns false,
     doing nothing to `point'.  Otherwise, it returns true. */
{
  if (v1 == v2)
    return false;
  
  // First find t such that the vector from point to v1+t(v2-v1) is orthogonal
  // to the line from v1 to v2.  Then evaluate v1+t(v2-v1).
  double Ax=v1.x, Ay=v1.y, Bx=v2.x, By=v2.y;
  double den = (Bx-Ax)*(Bx-Ax) + (By-Ay)*(By-Ay);
  double num = (point.x-Ax)*(Bx-Ax) + (point.y-Ay)*(By-Ay);
  double t = num/den; // `den' cannot be 0 because `v1' != `v2'
  double x = Ax+t*(Bx-Ax), y = Ay+t*(By-Ay);
  point.x = (int) floor(0.5+x);
  point.y = (int) floor(0.5+y);
  return true;
}

/*****************************************************************************/
/* STATIC               jx_find_path_edge_intersection                       */
/*****************************************************************************/

static bool
  jx_find_path_edge_intersection(kdu_coords *A, kdu_coords *B, kdu_coords *C,
                                 double delta, kdu_coords *pt)
  /* On entry, the first 3 arguments define two connected line segments:
     (A,B) and (B,C).  If `C' is NULL, line segment (B,C) is taken to be
     parallel to line segment (A,B) -- see below.  If either line segment
     has zero length, the function returns false.  The function displaces
     each line segment by an amount delta in the rightward perpendicular
     direction and finds their new intersection, rounding its coordinates to
     the nearest integer and returning them via `pt'.  If the line segments
     happen to be parallel, the function simply displaces point B by `delta'
     in the rightward direction, perpendicular to (A,B), rounds its
     coordinates and returns them via `pt'.  The function returns true in
     all cases, except if A=B or B=C, as indicated earlier, or if the
     intersection lies beyond pont A or beyond point C. */
{
  if ((A == NULL) || (B == NULL))
    return false;
  double ABx, ABy, BCx, BCy;
  BCx=ABx=B->x-A->x;
  BCy=ABy=B->y-A->y;
  if (C != NULL)
    { 
      BCx=C->x-B->x;
      BCy=C->y-B->y;
    }
  double AB_len = sqrt(ABx*ABx+ABy*ABy);
  double BC_len = sqrt(BCx*BCx+BCy*BCy);
  if ((AB_len < 0.1) || (BC_len < 0.1))
    return false;
  double p1_x=B->x-ABy*delta/AB_len, p1_y=B->y+ABx*delta/AB_len;
  double p2_x=B->x-BCy*delta/BC_len, p2_y=B->y+BCx*delta/BC_len;
  
  // Now solve p1 - t*AB = p2 + u*BC for some t<1 and u<1.
  // That is, p1-p2 = AB*t + BC*u.
  double m11=ABx, m12=BCx, m21=ABy, m22=BCy;
  double det = m11*m22-m12*m21;
  if ((det < 0.1) && (det > -0.1))
    { // The lines are parallel or almost parallel.
      pt->x = (int) floor(0.5+0.5*(p1_x+p2_x));
      pt->y = (int) floor(0.5+0.5*(p1_y+p2_y));
      return true;
    }
  double t = (m22*(p1_x-p2_x) - m12*(p1_y-p2_y)) / det;
  double u = (m11*(p1_y-p2_y) - m21*(p1_x-p2_x)) / det;
  pt->x = (int) floor(0.5 + p1_x-t*ABx);
  pt->y = (int) floor(0.5 + p1_y-t*ABy);
  return ((t < 1) && (u < 1));
}

/*****************************************************************************/
/* STATIC                 jx_check_line_intersection                         */
/*****************************************************************************/

static bool
  jx_check_line_intersection(const kdu_coords &A, const kdu_coords &B,
                             bool open_at_A, bool open_at_B,
                             const kdu_coords &C, const kdu_coords &D)
  /* This function determines whether or not the line segment from A->B
     intersects with (or even touches) the line segment from C->D.  Both
     line segments are assumed to have non-zero length (i.e., A != B and
     C != D).  The lines could potentially be parallel, in which case, the
     function still checks whether they intersect or touch.  If `open_at_A'
     is true, the single point A is not considered to be long to the line
     segment A->B, so the function can return false even if A belongs to
     C->D.  Similarly, if `open_at_B' is true, the single point B is not
     considered to belong to the line segment A->B and B is allowed to
     touch C->D. */
{
  // Write x = B + t'(A-B) for a point in [A,B] where 0 <= t' <= 1.
  // Write y = D + u'(C-D) for a point in [C,D] where 0 <= u' <= 1.
  // So if x = y for some t and u, we must have:
  // [(A-B)  (D-C)] * [t' u']^t = (D-B).
  kdu_long AmB_x = A.x-B.x, AmB_y=A.y-B.y;
  kdu_long DmC_x = D.x-C.x, DmC_y=D.y-C.y;
  kdu_long det = AmB_x*DmC_y - DmC_x*AmB_y;
  kdu_long DmB_x=D.x-B.x, DmB_y = D.y-B.y;
  kdu_long t = DmC_y*DmB_x - DmC_x*DmB_y; // t = t'*det
  kdu_long u = AmB_x*DmB_y - AmB_y*DmB_x; // u = u'*det
  if (det == 0)
    { // Line segments [A,B] and [C,D] are parallel.  It is still possible
      // to have an intersection, although rather unlikely.
      if (AmB_x == 0)
        { // The line segments are co-linear if their x-coords agree
          if (C.x != A.x)
            return false; // not co-linear
          assert(AmB_y != 0);
          // If t' = DmB_y/AmB_y and u' = CmB_y/AmB_y, then intersection
          // occurs if: a) 0 <= t' <= 1 (with strict inequalities if open at
          // B and A, respectively); b) 0 <= u' <= 1 (with strict inequalities
          // if open at B and A, respectively); c) t' <= 0 and u' >= 1; or
          // d) u' <= 0 and t' >= 1.
          det = AmB_y;
          t = DmB_y; // t = t'*det
          u = C.y-B.y; // u = u'*det
        }
      else
        { // Let t' = DmB_x / AmB_x and u' = CmB_x / AmB_x.  Then the line
          // segments are co-linear if t = DmB_y / AmB_y.  That is, we must
          // have t'*AmB_y = DmB_y.
          det = AmB_x;
          t = DmB_x; // t = t'*det
          u = C.x-B.x; // u = u'*det
          if ((t*AmB_y) != (det*DmB_y))
            return false; // Line segments are not co-linear
          // If we get here, then intersetion ocurrs under the same four
          // conditions (a) through (d) which were described above for the
          // AmB_x = 0 case.
        }
      if (det < 0)
        { det = -det; t = -t; u = -u; }
      if (((t >= 0) && !(open_at_B && (t==0))) &&
          ((t <= det) && !(open_at_A && (t==det))))
        return true;
      else if (((u >= 0) && !(open_at_B && (u==0))) &&
               ((u <= det) && !(open_at_A && (u==det))))
        return true;
      else if (((t <= 0) && (u >= det)) || ((u <= 0) && (t >= det)))
        return true; // Cases (c) and (d)
    }
  else
    { // Regular case of non-parallel lines
      if (det < 0)
        { det=-det; t=-t; u=-u; }
      if ((u < 0) || (u > det))
        return false;
      if ((t < 0) || (open_at_B && (t == 0)))
        return false;
      if ((t > det) || (open_at_A && (t == det)))
        return false;
      return true;
    }
  return false;
}


/* ========================================================================= */
/*                              jx_fragment_list                             */
/* ========================================================================= */

/*****************************************************************************/
/*                           jx_fragment_list::init                          */
/*****************************************************************************/

bool
  jx_fragment_list::init(jp2_input_box *flst, bool allow_errors)
{
  assert(flst->get_box_type() == jp2_fragment_list_4cc);
  total_length = 0;  num_frags = 0; // Just to be sure
  kdu_uint16 nf;
  if (!flst->read(nf))
    {
      if (!allow_errors)
        return false;
      else
        {
          KDU_ERROR(e,0); e <<
          KDU_TXT("Error encountered reading fragment list (flst) box.  "
                  "Unable to read the initial fragment count.");
        }
    }
  jpx_fragment_list ifc(this);
  for (; nf > 0; nf--)
    {
      kdu_uint16 url_idx;
      kdu_uint32 off1, off2, len;
      if (!(flst->read(off1) && flst->read(off2) && flst->read(len) &&
            flst->read(url_idx)))
        {
          if (!allow_errors)
            return false;
          else
            {
              KDU_ERROR(e,1); e <<
              KDU_TXT("Error encountered reading fragment list (flst) "
                      "box.  Contents of box terminated prematurely.");
            }
        }
      kdu_long offset = (kdu_long) off2;
#ifdef KDU_LONG64
      offset += ((kdu_long) off1) << 32;
#endif // KDU_LONG64
      ifc.add_fragment((int) url_idx,offset,(kdu_long) len);
    }
  flst->close();
  return true;
}

/*****************************************************************************/
/*                      jx_fragment_list::get_link_region                    */
/*****************************************************************************/

kdu_long jx_fragment_list::get_link_region(kdu_long &len)
{
  if (num_frags < 1)
    return -1;
  int n;
  kdu_long pos, start = frags[0].offset;
  for (pos=start, n=0; n < num_frags; pos+=frags[n].length, n++)
    if ((frags[n].url_idx != 0) || (frags[n].offset != pos))
      return -1;
  assert(pos == (start+total_length));
  len = total_length;
  return start;
}

/*****************************************************************************/
/*                          jx_fragment_list::save_box                       */
/*****************************************************************************/

void
  jx_fragment_list::save_box(jp2_output_box *super_box)
{
  int n, actual_box_frags = num_frags;
#ifdef KDU_LONG64
  for (n=0; n < num_frags; n++)
    if ((frags[n].length >> 32) > 0)
      actual_box_frags += (int)((frags[n].length-1) / 0x00000000FFFFFFFF);
#endif // KDU_LONG64
  if (actual_box_frags >= (1<<16))
    { KDU_ERROR_DEV(e,2); e <<
        KDU_TXT("Trying to write too many fragments to a fragment "
        "list (flst) box.  Maximum number of fragments is 65535, but note "
        "that each written fragment must have a length < 2^32 bytes.  Very "
        "long fragments may thus need to be split, creating the appearance "
        "of a very large number of fragments.");
    }
  jp2_output_box flst;
  flst.open(super_box,jp2_fragment_list_4cc);
  flst.write((kdu_uint16) actual_box_frags);
  for (n=0; n < num_frags; n++)
    {
      int url_idx = frags[n].url_idx;
      kdu_long offset = frags[n].offset;
      kdu_long remaining_length = frags[n].length;
      kdu_uint32 frag_length;
      do {
          frag_length = (kdu_uint32) remaining_length;
#ifdef KDU_LONG64
            if ((remaining_length >> 32) > 0)
              frag_length = 0xFFFFFFFF;
          flst.write((kdu_uint32)(offset>>32));
          flst.write((kdu_uint32) offset);
#else // !KDU_LONG64
          flst.write((kdu_uint32) 0);
          flst.write((kdu_uint32) offset);
#endif // !KDU_LONG64
          flst.write(frag_length);
          flst.write((kdu_uint16) url_idx);
          remaining_length -= (kdu_long) frag_length;
          offset += (kdu_long) frag_length;
          actual_box_frags--;
        } while (remaining_length > 0);
    }
  flst.close();
  assert(actual_box_frags == 0);
}

/*****************************************************************************/
/*                          jx_fragment_list::finalize                       */
/*****************************************************************************/

void
  jx_fragment_list::finalize(jp2_data_references data_references)
{
  int n, url_idx;
  for (n=0; n < num_frags; n++)
    {
      url_idx = frags[n].url_idx;
      if (data_references.get_url(url_idx) == NULL)
        { KDU_ERROR(e,3); e <<
            KDU_TXT("Attempting to read or write a fragment list "
            "box which refers to one or more URL's, not found in the "
            "associated data references object "
            "(see `jpx_target::access_data_references').");
        }
    }
}


/* ========================================================================= */
/*                              jpx_fragment_list                            */
/* ========================================================================= */

/*****************************************************************************/
/*                        jpx_fragment_list::add_fragment                    */
/*****************************************************************************/

void
  jpx_fragment_list::add_fragment(int url_idx, kdu_long offset,
                                  kdu_long length)
{
  if (state->num_frags == state->max_frags)
    { // Augment fragment array
      state->max_frags += state->num_frags + 8;
      jx_frag *tmp_frags = new jx_frag[state->max_frags];
      if (state->frags != NULL)
        {
          for (int n=0; n < state->num_frags; n++)
            tmp_frags[n] = state->frags[n];
          delete[] state->frags;
        }
      state->frags = tmp_frags;
    }

  // See if we can join the fragment to the last one
  jx_frag *frag = state->frags+state->num_frags-1;
  if ((frag >= state->frags) && (frag->url_idx == url_idx) &&
      ((frag->offset+frag->length) == offset))
    frag->length += length;
  else
    {
      frag++;
      state->num_frags++;
      frag->url_idx = url_idx;
      frag->offset = offset;
      frag->length = length;
    }
  state->total_length += length;
}

/*****************************************************************************/
/*                      jpx_fragment_list::get_total_length                  */
/*****************************************************************************/

kdu_long
  jpx_fragment_list::get_total_length()
{
  return (state==NULL)?0:(state->total_length);
}

/*****************************************************************************/
/*                      jpx_fragment_list::get_num_fragments                 */
/*****************************************************************************/

int
  jpx_fragment_list::get_num_fragments()
{
  return (state==NULL)?0:(state->num_frags);
}

/*****************************************************************************/
/*                        jpx_fragment_list::get_fragment                    */
/*****************************************************************************/

bool
  jpx_fragment_list::get_fragment(int frag_idx, int &url_idx,
                                  kdu_long &offset, kdu_long &length)
{
  if ((frag_idx < 0) || (frag_idx >= state->num_frags))
    return false;
  jx_frag *frag = state->frags + frag_idx;
  url_idx = frag->url_idx;
  offset = frag->offset;
  length = frag->length;
  return true;
}

/*****************************************************************************/
/*                     jpx_fragment_list::locate_fragment                    */
/*****************************************************************************/

int
  jpx_fragment_list::locate_fragment(kdu_long pos, kdu_long &bytes_into_frag)
{
  if (pos < 0)
    return -1;
  int idx;
  for (idx=0; idx < state->num_frags; idx++)
    {
      bytes_into_frag = pos;
      pos -= state->frags[idx].length;
      if (pos < 0)
        return idx;
    }
  return -1;
}


/* ========================================================================= */
/*                               jpx_input_box                               */
/* ========================================================================= */

/*****************************************************************************/
/*                       jpx_input_box::jpx_input_box                        */
/*****************************************************************************/

jpx_input_box::jpx_input_box()
{
  frag_idx=last_url_idx=-1;  frag_start=frag_lim=0;
  url_pos=last_url_pos=0; flst_src=NULL;  frag_file=NULL;
  path_buf=NULL; max_path_len = 0;
}

/*****************************************************************************/
/*                         jpx_input_box::open_next                          */
/*****************************************************************************/

bool jpx_input_box::open_next()
{
  if (flst_src != NULL)
    return false;
  else
    return jp2_input_box::open_next();
}

/*****************************************************************************/
/*                          jpx_input_box::open_as                           */
/*****************************************************************************/

bool
  jpx_input_box::open_as(jpx_fragment_list frag_list,
                         jp2_data_references data_references,
                         jp2_family_src *ultimate_src,
                         kdu_uint32 box_type)
{
  if (is_open)
    { KDU_ERROR_DEV(e,4); e <<
        KDU_TXT("Attempting to call `jpx_input_box::open_as' without "
        "first closing the box.");
    }
  if (ultimate_src == NULL)
    { KDU_ERROR_DEV(e,5); e <<
        KDU_TXT("You must supply a non-NULL `ultimate_src' argument "
        "to `jpx_input_box::open_as'.");
    }

  this->frag_list = frag_list;
  this->flst_src = ultimate_src;
  if (flst_src->cache == NULL)
    this->data_references = data_references; // Prevents any data being read
                    // at all if the fragment list belongs to a caching data
                    // source.  This is the safest way to ensure that we
                    // don't have to wait (possibly indefinitely) for a data
                    // references box to be delivered by a JPIP server.  JPIP
                    // servers should not send fragment lists anyway; they
                    // should use the special JPIP equivalent box mechanism.

  this->locator = jp2_locator();
  this->super_box = NULL;
  this->src = NULL;

  this->box_type = box_type;
  this->original_box_length = frag_list.get_total_length();
  this->original_header_length = 0;
  this->original_pos_offset = 0;
  this->next_box_offset = 0;
  this->contents_start = 0;
  this->contents_lim = frag_list.get_total_length();
  this->bin_id = -1;
  this->codestream_min = this->codestream_lim = this->codestream_id = -1;
  this->bin_class = 0;
  this->can_dereference_contents = false;
  this->rubber_length = false;
  this->is_open = true;
  this->is_locked = false;
  this->capabilities = KDU_SOURCE_CAP_SEQUENTIAL | KDU_SOURCE_CAP_SEEKABLE;
  this->pos = 0;
  this->partial_word_bytes = 0;
  frag_file = NULL;
  frag_idx = -1;
  last_url_idx = -1;
  frag_start = frag_lim = url_pos = last_url_pos = 0;
  return true;
}

/*****************************************************************************/
/*                           jpx_input_box::close                            */
/*****************************************************************************/

bool
  jpx_input_box::close()
{
  if (frag_file != NULL)
    { fclose(frag_file); frag_file = NULL; }
  if (path_buf != NULL)
    { delete[] path_buf; path_buf = NULL; }
  max_path_len = 0;
  frag_idx = -1;
  last_url_idx = -1;
  frag_start = frag_lim = url_pos = last_url_pos = 0;
  flst_src = NULL;
  frag_list = jpx_fragment_list(NULL);
  data_references = jp2_data_references(NULL);
  return jp2_input_box::close();
}

/*****************************************************************************/
/*                           jpx_input_box::seek                             */
/*****************************************************************************/

bool
  jpx_input_box::seek(kdu_long offset)
{
  if ((flst_src == NULL) || (contents_block != NULL))
    return jp2_input_box::seek(offset);
  assert(contents_start == 0);
  if (pos == offset)
    return true;
  kdu_long old_pos = pos;
  if (offset < 0)
    pos = 0;
  else
    pos = (offset < contents_lim)?offset:contents_lim;
  if ((frag_idx >= 0) && (pos >= frag_start) && (pos < frag_lim))
    { // Seeking within existing fragment
      url_pos += (pos-old_pos);
    }
  else
    { // Force re-determination of the active fragment
      frag_idx = -1;
      frag_start = frag_lim = url_pos = 0;
    }
  return true;
}

/*****************************************************************************/
/*                           jpx_input_box::read                             */
/*****************************************************************************/

int
  jpx_input_box::read(kdu_byte *buf, int num_bytes)
{
  if ((flst_src == NULL) || (contents_block != NULL))
    return jp2_input_box::read(buf,num_bytes);

  int result = 0;
  while (num_bytes > 0)
    {
      if ((frag_idx < 0) || (pos >= frag_lim))
        { // Find the fragment containing `pos'
          int url_idx;
          kdu_long bytes_into_fragment, frag_length;
          frag_idx = frag_list.locate_fragment(pos,bytes_into_fragment);
          if ((frag_idx < 0) ||
              !frag_list.get_fragment(frag_idx,url_idx,url_pos,frag_length))
            { // Cannot read any further
              frag_idx = -1;
              return result;
            }
          url_pos += bytes_into_fragment;
          frag_start = pos - bytes_into_fragment;
          frag_lim = frag_start + frag_length;
          if (url_idx != last_url_idx)
            { // Change the URL
              if (frag_file != NULL)
                {
                  fclose(frag_file);
                  frag_file = NULL;
                  last_url_idx = -1;
                }
              const char *file_url = "";
              if (url_idx != 0)
                {
                  if ((!data_references) ||
                      ((file_url =
                        data_references.get_file_url(url_idx)) == NULL) ||
                      ((*file_url != '\0') &&
                       ((frag_file=url_fopen(file_url)) == NULL)))
                    { // Cannot read any further
                      frag_idx = -1;
                      return result;
                    }
                }
              last_url_pos = 0;
              last_url_idx = url_idx;
            }
        }
      if (url_pos != last_url_pos)
        {
          if (frag_file != NULL) // Don't have to seek `kdu_family_src'
            kdu_fseek(frag_file,url_pos);
          last_url_pos = url_pos;
        }

      int xfer_bytes = num_bytes;
      if ((pos+xfer_bytes) > frag_lim)
        xfer_bytes = (int)(frag_lim-pos);
      if (frag_file != NULL)
        { // Read from `frag_file'
          xfer_bytes = (int) fread(buf,1,(size_t) xfer_bytes,frag_file);
        }
      else
        { // Read from `flst_src'
          if (flst_src->cache != NULL)
            return result;
          flst_src->acquire_lock();
          if (flst_src->last_read_pos != pos)
            {
              if (flst_src->fp != NULL)
                kdu_fseek(flst_src->fp,pos);
              else if (flst_src->indirect != NULL)
                flst_src->indirect->seek(pos);
            }
          if (flst_src->fp != NULL)
            xfer_bytes = (int) fread(buf,1,(size_t) xfer_bytes,flst_src->fp);
          else if (flst_src->indirect != NULL)
            xfer_bytes = flst_src->indirect->read(buf,xfer_bytes);
          flst_src->last_read_pos = pos + xfer_bytes;
          flst_src->release_lock();
        }

      if (xfer_bytes == 0)
        break; // Cannot read any further

      num_bytes -= xfer_bytes;
      result += xfer_bytes;
      buf += xfer_bytes;
      pos += xfer_bytes;
      url_pos += xfer_bytes;
      last_url_pos += xfer_bytes;
    }
  return result;
}

/*****************************************************************************/
/*                          jpx_input_box::url_fopen                         */
/*****************************************************************************/

FILE *
  jpx_input_box::url_fopen(const char *path)
{
  // First step is to determine whether or not this is a relative URL
  bool is_absolute = false;
  if ((path[0] == '.') && ((path[1] == '/') || (path[1] == '\\')))
    path += 2; // Walk over relative prefix
  else
    is_absolute = true;
  if (!is_absolute)
    {
      const char *rel_fname = flst_src->get_filename();
      if (rel_fname == NULL)
        return NULL;
      int len = ((int)(strlen(rel_fname) + strlen(path))) + 2;
      if (len > max_path_len)
        {
          max_path_len += len;
          if (path_buf != NULL)
            delete[] path_buf;
          path_buf = new char[max_path_len];
        }
      strcpy(path_buf,rel_fname);
      char *cp = path_buf+strlen(path_buf);
      while ((cp > path_buf) && (cp[-1] != '/') && (cp[-1] != '\\'))
        cp--;
      strcpy(cp,path);
      path = path_buf;
    }
  return fopen(path,"rb");
}


/* ========================================================================= */
/*                              jx_compatibility                             */
/* ========================================================================= */

/*****************************************************************************/
/*                        jx_compatibility::init_ftyp                        */
/*****************************************************************************/

bool
  jx_compatibility::init_ftyp(jp2_input_box *ftyp)
{
  assert(ftyp->get_box_type() == jp2_file_type_4cc);
  kdu_uint32 brand, minor_version, compat;
  ftyp->read(brand); ftyp->read(minor_version);

  bool jp2_compat=false, jpx_compat=false, jpxb_compat=false;
  while (ftyp->read(compat))
    if (compat == jp2_brand)
      jp2_compat = true;
    else if (compat == jpx_brand)
      jpx_compat = true;
    else if (compat == jpx_baseline_brand)
      jpxb_compat = jpx_compat = true;

  if (!ftyp->close())
    { KDU_ERROR(e,6); e <<
        KDU_TXT("JP2-family data source contains a malformed file type box.");
    }
  if (!(jp2_compat || jpx_compat))
    return false;
  this->is_jp2 = (brand == jp2_brand) || (!jpx_compat);
  this->is_jp2_compatible = jp2_compat;
  this->is_jpxb_compatible = jpxb_compat;
  this->have_rreq_box = false; // Until we find one
  return true;
}

/*****************************************************************************/
/*                        jx_compatibility::init_rreq                        */
/*****************************************************************************/

void
  jx_compatibility::init_rreq(jp2_input_box *rreq)
{
  assert(rreq->get_box_type() == jp2_reader_requirements_4cc);
  kdu_byte m, byte, mask_length = 0;
  int n, shift, m_idx;
  rreq->read(mask_length);
  for (shift=24, m_idx=0, m=0; (m < mask_length) && (m < 32); m++, shift-=8)
    {
      if (shift < 0)
        { shift=24; m_idx++; }
      rreq->read(byte);
      fully_understand_mask[m_idx] |= (((kdu_uint32) byte) << shift);
    }
  for (shift=24, m_idx=0, m=0; (m < mask_length) && (m<32); m++, shift-=8)
    {
      if (shift < 0)
        { shift=24; m_idx++; }
      rreq->read(byte);
      decode_completely_mask[m_idx] |= (((kdu_uint32) byte) << shift);
    }

  kdu_uint16 nsf;
  if (!rreq->read(nsf))
    { KDU_ERROR(e,7); e <<
        KDU_TXT("Malformed reader requirements (rreq) box found in "
        "JPX data source.  Box terminated unexpectedly.");
    }
  have_rreq_box = true;
  num_standard_features = max_standard_features = (int) nsf;
  standard_features = new jx_feature[max_standard_features];
  for (n=0; n < num_standard_features; n++)
    {
      jx_feature *fp = standard_features + n;
      rreq->read(fp->feature_id);
      for (shift=24, m_idx=0, m=0; (m < mask_length) && (m<32); m++, shift-=8)
        {
          if (shift < 0)
            { shift=24; m_idx++; }
          rreq->read(byte);
          fp->mask[m_idx] |= (((kdu_uint32) byte) << shift);
        }
      fp->supported = true;
      if (fp->feature_id == JPX_SF_CODESTREAM_FRAGMENTS_REMOTE)
        fp->supported = false; // The only one we know we don't support.
    }

  kdu_uint16 nvf;
  if (!rreq->read(nvf))
    { KDU_ERROR(e,8); e <<
        KDU_TXT("Malformed reader requirements (rreq) box found in "
        "JPX data source.  Box terminated unexpectedly.");
    }
  num_vendor_features = max_vendor_features = (int) nvf;
  vendor_features = new jx_vendor_feature[max_vendor_features];
  for (n=0; n < num_vendor_features; n++)
    {
      jx_vendor_feature *fp = vendor_features + n;
      if (rreq->read(fp->uuid,16) != 16)
        { KDU_ERROR(e,9); e <<
            KDU_TXT("Malformed reader requirements (rreq) box found "
            "in JPX data source. Box terminated unexpectedly.");
        }
      for (shift=24, m_idx=0, m=0; (m < mask_length) && (m<32); m++, shift-=8)
        {
          if (shift < 0)
            { shift=24; m_idx++; }
          if (!rreq->read(byte))
            { KDU_ERROR(e,10); e <<
                KDU_TXT("Malformed reader requirements (rreq) box "
                "found in JPX data source. Box terminated unexpectedly.");
            }
          fp->mask[m_idx] |= (((kdu_uint32) byte) << shift);
        }
      fp->supported = false;
    }
  if (!rreq->close())
    { KDU_ERROR(e,11); e <<
        KDU_TXT("Malformed reader requirements (rreq) box "
        "found in JPX data source.  Box appears to be too long.");
    }
}

/*****************************************************************************/
/*                     jx_compatibility::copy_from                           */
/*****************************************************************************/

void
  jx_compatibility::copy_from(jx_compatibility *src)
{
  jpx_compatibility src_ifc(src);
  is_jp2_compatible = is_jp2_compatible && src->is_jp2_compatible;
  is_jpxb_compatible = is_jpxb_compatible && src->is_jpxb_compatible;
  no_extensions = no_extensions &&
    src_ifc.check_standard_feature(JPX_SF_CODESTREAM_NO_EXTENSIONS);
  no_opacity = no_opacity &&
    src_ifc.check_standard_feature(JPX_SF_NO_OPACITY);
  no_fragments = no_fragments &&
    src_ifc.check_standard_feature(JPX_SF_CODESTREAM_CONTIGUOUS);
  no_scaling = no_scaling &&
    src_ifc.check_standard_feature(JPX_SF_NO_SCALING);
  single_stream_layers = single_stream_layers &&
    src_ifc.check_standard_feature(JPX_SF_ONE_CODESTREAM_PER_LAYER);
  int n, k;
  for (n=0; n < src->num_standard_features; n++)
    {
      jx_feature *fp, *feature = src->standard_features + n;
      if ((feature->feature_id == JPX_SF_CODESTREAM_NO_EXTENSIONS) ||
          (feature->feature_id == JPX_SF_NO_OPACITY) ||
          (feature->feature_id == JPX_SF_CODESTREAM_CONTIGUOUS) ||
          (feature->feature_id == JPX_SF_NO_SCALING) ||
          (feature->feature_id == JPX_SF_ONE_CODESTREAM_PER_LAYER))
        continue;
      for (fp=standard_features, k=0; k < num_standard_features; k++, fp++)
        if (fp->feature_id == feature->feature_id)
          break;
      if (k == num_standard_features)
        { // Add a new feature
          if (k == max_standard_features)
            {
              max_standard_features += k + 10;
              fp = new jx_feature[max_standard_features];
              for (k=0; k < num_standard_features; k++)
                fp[k] = standard_features[k];
              if (standard_features != NULL)
                delete[] standard_features;
              standard_features = fp;
              fp += k;
            }
          num_standard_features++;
          fp->feature_id = feature->feature_id;
        }
      for (k=0; k < 8; k++)
        {
          fp->fully_understand[k] |= feature->fully_understand[k];
          fp->decode_completely[k] |= feature->decode_completely[k];
          fully_understand_mask[k] |= fp->fully_understand[k];
          decode_completely_mask[k] |= fp->decode_completely[k];
        }
    }

  for (n=0; n < src->num_vendor_features; n++)
    {
      jx_vendor_feature *fp, *feature = src->vendor_features + n;
      for (fp=vendor_features, k=0; k < num_vendor_features; k++, fp++)
        if (memcmp(fp->uuid,feature->uuid,16) == 0)
          break;
      if (k == num_vendor_features)
        { // Add a new feature
          if (k == max_vendor_features)
            {
              max_vendor_features += k + 10;
              fp = new jx_vendor_feature[max_vendor_features];
              for (k=0; k < num_vendor_features; k++)
                fp[k] = vendor_features[k];
              if (vendor_features != NULL)
                delete[] vendor_features;
              vendor_features = fp;
              fp += k;
            }
          num_vendor_features++;
          memcpy(fp->uuid,feature->uuid,16);
        }
      for (k=0; k < 8; k++)
        {
          fp->fully_understand[k] |= feature->fully_understand[k];
          fp->decode_completely[k] |= feature->decode_completely[k];
          fully_understand_mask[k] |= fp->fully_understand[k];
          decode_completely_mask[k] |= fp->decode_completely[k];
        }
    }
}

/*****************************************************************************/
/*                  jx_compatibility::add_standard_feature                   */
/*****************************************************************************/

void
  jx_compatibility::add_standard_feature(kdu_uint16 feature_id,
                                         bool add_to_both)
{
  int n;
  jx_feature *fp = standard_features;
  for (n=0; n < num_standard_features; n++, fp++)
    if (fp->feature_id == feature_id)
      break;
  if (n < num_standard_features)
    return; // Feature requirements already set by application

  // Create the feature
  if (max_standard_features == n)
    {
      max_standard_features += n + 10;
      fp = new jx_feature[max_standard_features];
      for (n=0; n < num_standard_features; n++)
        fp[n] = standard_features[n];
      if (standard_features != NULL)
        delete[] standard_features;
      standard_features = fp;
      fp += n;
    }
  num_standard_features++;
  fp->feature_id = feature_id;
  
  // Find an unused "fully understand" sub-expression index and use it
  int m;
  kdu_uint32 mask, subx;

  for (m=0; m < 8; m++)
    if ((mask = fully_understand_mask[m]) != 0xFFFFFFFF)
      break;
  if (m < 8)
    { // Otherwise, we are all out of sub-expressions: highly unlikely.
      for (subx=1<<31; subx & mask; subx>>=1);
      fully_understand_mask[m] |= subx;
      fp->fully_understand[m] |= subx;
    }

  if (add_to_both)
    { // Find an unused "decode completely" sub-expression index and use it
      for (m=0; m < 8; m++)
        if ((mask = decode_completely_mask[m]) != 0xFFFFFFFF)
          break;
      if (m < 8)
        { // Otherwise, we are all out of sub-expressions: highly unlikely.
          for (subx=1<<31; subx & mask; subx>>=1);
          decode_completely_mask[m] |= subx;
          fp->decode_completely[m] |= subx;
        }
    }

  // Check for features which will negate any of the special negative features
  if ((feature_id == JPX_SF_OPACITY_NOT_PREMULTIPLIED) ||
      (feature_id == JPX_SF_OPACITY_BY_CHROMA_KEY))
    no_opacity = false;
  if ((feature_id == JPX_SF_CODESTREAM_FRAGMENTS_INTERNAL_AND_ORDERED) ||
      (feature_id == JPX_SF_CODESTREAM_FRAGMENTS_INTERNAL) ||
      (feature_id == JPX_SF_CODESTREAM_FRAGMENTS_LOCAL) ||
      (feature_id == JPX_SF_CODESTREAM_FRAGMENTS_REMOTE))
    no_fragments = false;
  if ((feature_id == JPX_SF_SCALING_WITHIN_LAYER) ||
      (feature_id == JPX_SF_SCALING_BETWEEN_LAYERS))
    no_scaling = false;
  if (feature_id == JPX_SF_MULTIPLE_CODESTREAMS_PER_LAYER)
    single_stream_layers = false;
}

/*****************************************************************************/
/*                       jx_compatibility::save_boxes                        */
/*****************************************************************************/

void
  jx_compatibility::save_boxes(jx_target *owner)
{
  // Start by writing the file-type box
  owner->open_top_box(&out,jp2_file_type_4cc);
  assert(!is_jp2); // We can read JP2 or JPX, but only write JPX.
  out.write(jpx_brand);
  out.write((kdu_uint32) 0);
  out.write(jpx_brand);
  if (is_jp2_compatible)
    out.write(jp2_brand);
  if (is_jpxb_compatible)
    out.write(jpx_baseline_brand);
  out.close();

  // Now for the reader requirements box.  First, set negative features
  assert(have_rreq_box);
  if (no_extensions)
    add_standard_feature(JPX_SF_CODESTREAM_NO_EXTENSIONS);
  if (no_opacity)
    add_standard_feature(JPX_SF_NO_OPACITY);
  if (no_fragments)
    add_standard_feature(JPX_SF_CODESTREAM_CONTIGUOUS);
  if (no_scaling)
    add_standard_feature(JPX_SF_NO_SCALING);
  if (single_stream_layers)
    add_standard_feature(JPX_SF_ONE_CODESTREAM_PER_LAYER);
  
  // Next, we need to merge the fully understand and decode completely
  // sub-expressions, which may require quite a bit of manipulation.
  kdu_uint32 new_fumask[8] = {0,0,0,0,0,0,0,0};
  kdu_uint32 new_dcmask[8] = {0,0,0,0,0,0,0,0};
  int k, n, src_word, dst_word, src_shift, dst_shift;
  kdu_uint32 src_mask, dst_mask;
  int total_mask_bits=0;
  for (src_word=dst_word=0, src_mask=dst_mask=(1<<31); src_word < 8; )
    {
      if (fully_understand_mask[src_word] & src_mask)
        {
          for (n=0; n < num_standard_features; n++)
            {
              jx_feature *fp = standard_features + n;
              if (fp->fully_understand[src_word] & src_mask)
                fp->mask[dst_word] |= dst_mask;
            }
          for (n=0; n < num_vendor_features; n++)
            {
              jx_vendor_feature *fp = vendor_features + n;
              if (fp->fully_understand[src_word] & src_mask)
                fp->mask[dst_word] |= dst_mask;
            }
          new_fumask[dst_word] |= dst_mask;
          total_mask_bits++;
          if ((dst_mask >>= 1) == 0)
            { dst_mask = 1<<31; dst_word++; }
        }
      if ((src_mask >>= 1) == 0)
        { src_mask = 1<<31; src_word++; }
    }

  for (src_word=0, src_shift=31, src_mask=(1<<31); src_word < 8; )
    {
      if (decode_completely_mask[src_word] & src_mask)
        { // Scan existing sub-expressions looking for one which matches
          bool expressions_match = false;
          for (k=0, dst_word=0, dst_shift=31, dst_mask=(1<<31);
               k < total_mask_bits; k++)
            {
              expressions_match=true;
              for (n=0; (n < num_standard_features) && expressions_match; n++)
                {
                  jx_feature *fp = standard_features + n;
                  if ((((fp->decode_completely[src_word] >> src_shift) ^
                        (fp->mask[dst_word] >> dst_shift)) & 1) != 0)
                    expressions_match = false;
                }
              for (n=0; (n < num_vendor_features) && expressions_match; n++)
                {
                  jx_vendor_feature *fp = vendor_features + n;
                  if ((((fp->decode_completely[src_word] >> src_shift) ^
                        (fp->mask[dst_word] >> dst_shift)) & 1) != 0)
                    expressions_match = false;
                }
              dst_shift--; dst_mask >>= 1;
              if (dst_shift < 0)
                { dst_mask = 1<<31; dst_shift=31; dst_word++; }
            }
          if (!expressions_match)
            {
              if (total_mask_bits == 256)
                continue; // Not enough bits to store this new sub-expression
              total_mask_bits++; // This is a new sub-expression
            }
          new_dcmask[dst_word] |= dst_mask;
        }
      src_shift--; src_mask >>= 1;
      if (src_shift < 0)
        { src_mask = 1<<31; src_shift=31; src_word++; }
    }

  for (n=0; n < 8; n++)
    {
      fully_understand_mask[n] = new_fumask[n];
      decode_completely_mask[n] = new_dcmask[n];
    }

  // Now we are ready to write the reader requirements box
  owner->open_top_box(&out,jp2_reader_requirements_4cc);
  kdu_byte mask_length = (kdu_byte)((total_mask_bits+7)>>3);
  out.write(mask_length);
  for (k=mask_length, src_word=0, src_shift=24; k > 0; k--, src_shift-=8)
    {
      if (src_shift < 0)
        { src_shift = 24; src_word++; }
      out.write((kdu_byte)(fully_understand_mask[src_word] >> src_shift));
    }
  for (k=mask_length, src_word=0, src_shift=24; k > 0; k--, src_shift-=8)
    {
      if (src_shift < 0)
        { src_shift = 24; src_word++; }
      out.write((kdu_byte)(decode_completely_mask[src_word] >> src_shift));
    }
  out.write((kdu_uint16) num_standard_features);
  for (n=0; n < num_standard_features; n++)
    {
      jx_feature *fp = standard_features + n;
      out.write(fp->feature_id);
      for (k=mask_length, src_word=0, src_shift=24; k > 0; k--, src_shift-=8)
        {
          if (src_shift < 0)
            { src_shift = 24; src_word++; }
          out.write((kdu_byte)(fp->mask[src_word] >> src_shift));
        }
    }
  out.write((kdu_uint16) num_vendor_features);
  for (n=0; n < num_vendor_features; n++)
    {
      jx_vendor_feature *fp = vendor_features + n;
      for (k=0; k < 16; k++)
        out.write(fp->uuid[k]);

      for (k=mask_length, src_word=0, src_shift=24; k > 0; k--, src_shift-=8)
        {
          if (src_shift < 0)
            { src_shift = 24; src_word++; }
          out.write((kdu_byte)(fp->mask[src_word] >> src_shift));
        }
    }
  out.close();
}


/* ========================================================================= */
/*                             jpx_compatibility                             */
/* ========================================================================= */

/*****************************************************************************/
/*                         jpx_compatibility::is_jp2                         */
/*****************************************************************************/

bool
  jpx_compatibility::is_jp2()
{
  return (state != NULL) && state->is_jp2;
}

/*****************************************************************************/
/*                    jpx_compatibility::is_jp2_compatible                   */
/*****************************************************************************/

bool
  jpx_compatibility::is_jp2_compatible()
{
  return (state != NULL) && state->is_jp2_compatible;
}

/*****************************************************************************/
/*                    jpx_compatibility::is_jpxb_compatible                  */
/*****************************************************************************/

bool
  jpx_compatibility::is_jpxb_compatible()
{
  return (state != NULL) && state->is_jpxb_compatible;
}

/*****************************************************************************/
/*               jpx_compatibility::has_reader_requirements_box              */
/*****************************************************************************/

bool
  jpx_compatibility::has_reader_requirements_box()
{
  return (state != NULL) && state->have_rreq_box;
}

/*****************************************************************************/
/*                  jpx_compatibility::check_standard_feature                */
/*****************************************************************************/

bool
  jpx_compatibility::check_standard_feature(kdu_uint16 feature_id)
{
  if ((state == NULL) || !state->have_rreq_box)
    return false;
  for (int n=0; n < state->num_standard_features; n++)
    if (state->standard_features[n].feature_id == feature_id)
      return true;
  return false;
}

/*****************************************************************************/
/*                   jpx_compatibility::check_vendor_feature                 */
/*****************************************************************************/

bool
  jpx_compatibility::check_vendor_feature(kdu_byte uuid[])
{
  if ((state == NULL) || !state->have_rreq_box)
    return false;
  for (int n=0; n < state->num_vendor_features; n++)
    if (memcmp(state->vendor_features[n].uuid,uuid,16) == 0)
      return true;
  return false;
}

/*****************************************************************************/
/*                   jpx_compatibility::get_standard_feature                 */
/*****************************************************************************/

bool
  jpx_compatibility::get_standard_feature(int which, kdu_uint16 &feature_id)
{
  if ((state==NULL) || (!state->have_rreq_box) ||
      (which >= state->num_standard_features) || (which < 0))
    return false;
  feature_id = state->standard_features[which].feature_id;
  return true;
}

/*****************************************************************************/
/*              jpx_compatibility::get_standard_feature (extended)           */
/*****************************************************************************/

bool
  jpx_compatibility::get_standard_feature(int which, kdu_uint16 &feature_id,
                                          bool &is_supported)
{
  if ((state==NULL) || (!state->have_rreq_box) ||
      (which >= state->num_standard_features) || (which < 0))
    return false;
  feature_id = state->standard_features[which].feature_id;
  is_supported = state->standard_features[which].supported;
  return true;
}

/*****************************************************************************/
/*                    jpx_compatibility::get_vendor_feature                  */
/*****************************************************************************/

bool
  jpx_compatibility::get_vendor_feature(int which, kdu_byte uuid[])
{
  if ((state==NULL) || (!state->have_rreq_box) ||
      (which >= state->num_vendor_features) || (which < 0))
    return false;
  memcpy(uuid,state->vendor_features[which].uuid,16);
  return true;
}

/*****************************************************************************/
/*               jpx_compatibility::get_vendor_feature (extended)            */
/*****************************************************************************/

bool
  jpx_compatibility::get_vendor_feature(int which, kdu_byte uuid[],
                                        bool &is_supported)
{
  if ((state==NULL) || (!state->have_rreq_box) ||
      (which >= state->num_vendor_features) || (which < 0))
    return false;
  memcpy(uuid,state->vendor_features[which].uuid,16);
  is_supported = state->vendor_features[which].supported;
  return true;
}

/*****************************************************************************/
/*               jpx_compatibility::set_standard_feature_support             */
/*****************************************************************************/

void
  jpx_compatibility::set_standard_feature_support(kdu_uint16 feature_id,
                                                  bool is_supported)
{
  int n;
  if ((state != NULL) && state->have_rreq_box)
    {
      for (n=0; n < state->num_standard_features; n++)
        if (feature_id == state->standard_features[n].feature_id)
          {
            state->standard_features[n].supported = is_supported;
            break;
          }
    }
}

/*****************************************************************************/
/*                jpx_compatibility::set_vendor_feature_support              */
/*****************************************************************************/

void
  jpx_compatibility::set_vendor_feature_support(const kdu_byte uuid[],
                                                bool is_supported)
{
  int n;
  if ((state != NULL) && state->have_rreq_box)
    {
      for (n=0; n < state->num_vendor_features; n++)
        if (memcmp(uuid,state->vendor_features[n].uuid,16) == 0)
          {
            state->vendor_features[n].supported = is_supported;
            break;
          }
    }
}

/*****************************************************************************/
/*                  jpx_compatibility::test_fully_understand                 */
/*****************************************************************************/

bool
  jpx_compatibility::test_fully_understand()
{
  if (state == NULL)
    return false;
  if (!state->have_rreq_box)
    return true;

  int n, m;
  kdu_uint32 mask[8] = {0,0,0,0,0,0,0,0};
  for (n=0; n < state->num_standard_features; n++)
    {
      jx_compatibility::jx_feature *fp = state->standard_features + n;
      if (fp->supported)
        for (m=0; m < 8; m++)
          mask[m] |= fp->mask[m];
    }
  for (n=0; n < state->num_vendor_features; n++)
    {
      jx_compatibility::jx_vendor_feature *fp = state->vendor_features + n;
      if (fp->supported)
        for (m=0; m < 8; m++)
          mask[m] |= fp->mask[m];
    }
  for (m=0; m < 8; m++)
    if ((mask[m] & state->fully_understand_mask[m]) !=
        state->fully_understand_mask[m])
      return false;
  return true;
}

/*****************************************************************************/
/*                 jpx_compatibility::test_decode_completely                 */
/*****************************************************************************/

bool
  jpx_compatibility::test_decode_completely()
{
  if (state == NULL)
    return false;
  if (!state->have_rreq_box)
    return true;

  int n, m;
  kdu_uint32 mask[8] = {0,0,0,0,0,0,0,0};
  for (n=0; n < state->num_standard_features; n++)
    {
      jx_compatibility::jx_feature *fp = state->standard_features + n;
      if (fp->supported)
        for (m=0; m < 8; m++)
          mask[m] |= fp->mask[m];
    }
  for (n=0; n < state->num_vendor_features; n++)
    {
      jx_compatibility::jx_vendor_feature *fp = state->vendor_features + n;
      if (fp->supported)
        for (m=0; m < 8; m++)
          mask[m] |= fp->mask[m];
    }
  for (m=0; m < 8; m++)
    if ((mask[m] & state->decode_completely_mask[m]) !=
        state->decode_completely_mask[m])
      return false;
  return true;
}

/*****************************************************************************/
/*                       jpx_compatibility::copy                             */
/*****************************************************************************/

void
  jpx_compatibility::copy(jpx_compatibility src)
{
  state->copy_from(src.state);
}

/*****************************************************************************/
/*               jpx_compatibility::set_used_standard_feature                */
/*****************************************************************************/

void
  jpx_compatibility::set_used_standard_feature(kdu_uint16 feature_id,
                                               kdu_byte fusx_idx,
                                               kdu_byte dcsx_idx)
{
  if (state == NULL)
    return;
  state->have_rreq_box = true; // Just in case

  int n;
  jx_compatibility::jx_feature *fp = state->standard_features;
  for (n=0; n < state->num_standard_features; n++, fp++)
    if (fp->feature_id == feature_id)
      break;
  if (n == state->num_standard_features)
    { // Create the feature
      if (state->max_standard_features == n)
        {
          state->max_standard_features += n + 10;
          fp = new jx_compatibility::jx_feature[state->max_standard_features];
          for (n=0; n < state->num_standard_features; n++)
            fp[n] = state->standard_features[n];
          if (state->standard_features != NULL)
            delete[] state->standard_features;
          state->standard_features = fp;
          fp += n;
        }
      state->num_standard_features++;
    }

  fp->feature_id = feature_id;
  if (fusx_idx < 255)
    fp->fully_understand[fusx_idx >> 5] |= (1 << (31-(fusx_idx & 31)));
  if (dcsx_idx < 255)
    fp->decode_completely[dcsx_idx >> 5] |= (1 << (31-(dcsx_idx & 31)));
}

/*****************************************************************************/
/*                jpx_compatibility::set_used_vendor_feature                 */
/*****************************************************************************/

void
  jpx_compatibility::set_used_vendor_feature(const kdu_byte uuid[],
                                             kdu_byte fusx_idx,
                                             kdu_byte dcsx_idx)
{
  if (state == NULL)
    return;
  state->have_rreq_box = true; // Just in case

  int n;
  jx_compatibility::jx_vendor_feature *fp = state->vendor_features;
  for (n=0; n < state->num_vendor_features; n++, fp++)
    if (memcpy(fp->uuid,uuid,16) == 0)
      break;
  if (n == state->num_vendor_features)
    { // Create the feature
      if (state->max_vendor_features == n)
        {
          state->max_vendor_features += n + 10;
          fp = new
            jx_compatibility::jx_vendor_feature[state->max_vendor_features];
          for (n=0; n < state->num_vendor_features; n++)
            fp[n] = state->vendor_features[n];
          if (state->vendor_features != NULL)
            delete[] state->vendor_features;
          state->vendor_features = fp;
          fp += n;
        }
      state->num_vendor_features++;
    }

  memcpy(fp->uuid,uuid,16);
  if (fusx_idx < 255)
    {
      fp->fully_understand[fusx_idx>>5] |= (1<<(31-(fusx_idx & 31)));
      state->fully_understand_mask[fusx_idx>>5] |= (1<<(31-(fusx_idx & 31)));
    }
  if (dcsx_idx < 255)
    {
      fp->decode_completely[dcsx_idx>>5] |= (1<<(31-(dcsx_idx & 31)));
      state->decode_completely_mask[dcsx_idx>>5] |= (1<<(31-(dcsx_idx & 31)));
    }
}


/* ========================================================================= */
/*                               jx_composition                              */
/* ========================================================================= */

/*****************************************************************************/
/*                         jx_composition::add_frame                         */
/*****************************************************************************/

void
  jx_composition::add_frame()
{
  if (tail == NULL)
    {
      head = tail = new jx_frame;
      return;
    }
  if (tail->persistent)
    last_persistent_frame = tail;
  tail->next = new jx_frame;
  tail->next->prev = tail;
  tail = tail->next;
  tail->last_persistent_frame = last_persistent_frame;
  last_frame_max_lookahead = max_lookahead;
}

/*****************************************************************************/
/*                   jx_composition::donate_composition_box                  */
/*****************************************************************************/

void
  jx_composition::donate_composition_box(jp2_input_box &src,
                                         jx_source *owner)
{
  if (comp_in.exists())
    { KDU_WARNING(w,0); w <<
        KDU_TXT("JPX data source appears to contain multiple "
        "composition boxes!! This is illegal.  "
        "All but first will be ignored.");
      return;
    }
  comp_in.transplant(src);
  num_parsed_iset_boxes = 0;
  finish(owner);
}

/*****************************************************************************/
/*                           jx_composition::finish                          */
/*****************************************************************************/

bool
  jx_composition::finish(jx_source *owner)
{
  if (is_complete)
    return true;
  while ((!comp_in) && !owner->is_top_level_complete())
    if (!owner->parse_next_top_level_box())
      break;
  if (!comp_in)
    return owner->is_top_level_complete();

  assert(comp_in.get_box_type() == jp2_composition_4cc);
  if (!comp_in.is_complete())
    return false;
  
  while (sub_in.exists() || sub_in.open(&comp_in))
    {
      if (sub_in.get_box_type() == jp2_comp_options_4cc)
        {
          if (!sub_in.is_complete())
            break;
          kdu_uint32 height, width;
          kdu_byte loop;
          if (!(sub_in.read(height) && sub_in.read(width) &&
                sub_in.read(loop) && (height>0) && (width>0)))
            { KDU_ERROR(e,12); e <<
              KDU_TXT("Malformed Composition Options (copt) box "
                      "found in JPX data source.  Insufficient or illegal "
                      "field values encountered.  The height and width "
                      "parameters must also be non-zero.");
            }
          size.x = (int) width;
          size.y = (int) height;
          if (loop == 255)
            loop_count = 0;
          else
            loop_count = ((int) loop) + 1;
        }
      else if (sub_in.get_box_type() == jp2_comp_instruction_set_4cc)
        {
          if (!sub_in.is_complete())
            break;
          kdu_uint16 flags, rept;
          kdu_uint32 tick;
          if (!(sub_in.read(flags) && sub_in.read(rept) &&
                sub_in.read(tick)))
            { KDU_ERROR(e,13); e <<
              KDU_TXT("Malformed Instruction Set (inst) box "
                      "found in JPX data source.  Insufficient fields "
                      "encountered.");
            }
          bool have_target_pos = ((flags & 1) != 0);
          bool have_target_size = ((flags & 2) != 0);
          bool have_life_persist = ((flags & 4) != 0);
          bool have_source_region = ((flags & 32) != 0);
          if (!(have_target_pos || have_target_size ||
                have_life_persist || have_source_region))
            {
              sub_in.close();
              num_parsed_iset_boxes++;
              continue;
            }
          kdu_long start_pos = sub_in.get_pos();
          for (int repeat_count=rept; repeat_count >= 0; repeat_count--)
            {
              int inum_idx = 0;
              jx_frame *start_frame = tail;
              jx_instruction *start_tail = (tail==NULL)?NULL:(tail->tail);
              while (parse_instruction(have_target_pos,have_target_size,
                                       have_life_persist,
                                       have_source_region,tick))
                {
                  tail->tail->iset_idx = num_parsed_iset_boxes;
                  tail->tail->inum_idx = inum_idx++;
                }
              if (sub_in.get_remaining_bytes() > 0)
                { KDU_ERROR(e,14); e <<
                  KDU_TXT("Malformed Instruction Set (inst) box "
                          "encountered in JPX data source.  Box appears "
                          "to be too long.");
                }
              sub_in.seek(start_pos); // In case we need to repeat it all
              if (repeat_count < 2)
                continue;

              /* At this point, we have only to see if we can handle
                 repeats by adding a repeat count to the last frame.  For
                 this to work, we need to know that this instruction set
                 represents exactly one frame, that the frame is
                 non-persistent, and that the `next_use' pointers in the
                 frame either point beyond the repeat sequence, or to the
                 immediate next repetition instance.  Even then, we
                 cannot be sure that the first frame has exactly the same
                 number of reused layers as the remaining ones, so we will
                 have to make a copy of the frame and repeat only the
                 second copy. */
              if (tail == start_frame)
                continue; // Instruction set did not create a new frame
              if (tail->duration == 0)
                continue; // Instruction set did not complete a frame
              if ((start_frame == NULL) && (tail != head))
                continue; // Instruction set created multiple frames
              if ((start_frame != NULL) &&
                  ((tail != start_frame->next) ||
                   (start_frame->tail != start_tail)))
                continue; // Inst. set contributed to multiple frames
              
              jx_instruction *ip;
              int max_repeats = INT_MAX;
              if (last_frame_max_lookahead >= tail->num_instructions)
                max_repeats =
                  (last_frame_max_lookahead / tail->num_instructions) - 1;
              if (max_repeats == 0)
                continue;
              int remaining = tail->num_instructions;
              int reused_layers_in_frame = 0;
              for (ip=tail->head; ip != NULL; ip=ip->next, remaining--)
                {
                  if (ip->next_reuse == tail->num_instructions)
                    reused_layers_in_frame++; // Re-used in next frame
                  else if (ip->next_reuse != 0)
                    break;
                }
              if (ip != NULL)
                continue; // No repetitions

              // If we get here, we can repeat at most `max_repeats' times
              assert(remaining == 0);
              assert(max_repeats >= 0);
              if (max_repeats <= 1)
                continue; // Need at least 2 repetitions, since we cannot
                          // reliably start repetiting until 2'nd instance
              start_frame = tail;
              add_frame();
              repeat_count--;
              max_repeats--;
              max_lookahead -= tail->num_instructions;
              tail->increment =
              start_frame->num_instructions - reused_layers_in_frame;
              tail->persistent = start_frame->persistent;
              tail->duration = start_frame->duration;
              for (ip=start_frame->head; ip != NULL; ip=ip->next)
                {
                  jx_instruction *inst=tail->add_instruction(ip->visible);
                  inst->layer_idx = -1; // Evaluate this later on
                  inst->source_dims = ip->source_dims;
                  inst->target_dims = ip->target_dims;
                  if ((inst->next_reuse=ip->next_reuse) == 0)
                    inst->increment = tail->increment;
                }
              if (max_repeats == INT_MAX)
                { // No restriction on number of repetitions
                  if (rept == 0xFFFF)
                    tail->repeat_count = -1; // Repeat indefinitely
                  else
                    tail->repeat_count = repeat_count;
                  repeat_count = 0; // Finish looping
                }
              else
                {
                  if (rept == 0xFFFF)
                    tail->repeat_count = max_repeats;
                  else if (repeat_count > max_repeats)
                    {
                      tail->repeat_count = max_repeats;
                      repeat_count -= tail->repeat_count;
                    }
                  else
                    {
                      tail->repeat_count = repeat_count;
                      repeat_count = 0;
                    }
                }
              max_lookahead -= tail->repeat_count*tail->num_instructions;
              if (tail->repeat_count < 0)
                max_lookahead = 0;
            }
          num_parsed_iset_boxes++;
        }
      sub_in.close();
    }
  if (sub_in.exists())
    return false;
      
  comp_in.close();
  is_complete = true;

  assign_layer_indices();
  remove_invisible_instructions();
  return true;
}

/*****************************************************************************/
/*                     jx_composition::parse_instruction                     */
/*****************************************************************************/

bool
  jx_composition::parse_instruction(bool have_target_pos,
                                    bool have_target_size,
                                    bool have_life_persist,
                                    bool have_source_region,
                                    kdu_uint32 tick)
{
  if (!(have_target_pos || have_target_size ||
        have_life_persist || have_source_region))
    return false;

  kdu_dims source_dims, target_dims;
  if (have_target_pos)
    {
      kdu_uint32 X0, Y0;
      if (!sub_in.read(X0))
        return false;
      if (!sub_in.read(Y0))
        { KDU_ERROR(e,15); e <<
            KDU_TXT("Malformed Instruction Set (inst) box "
            "found in JPX data source.  Terminated unexpectedly.");
        }
      target_dims.pos.x = (int) X0;
      target_dims.pos.y = (int) Y0;
    }
  else
    target_dims.pos = kdu_coords(0,0);

  if (have_target_size)
    {
      kdu_uint32 XS, YS;
      if (!(sub_in.read(XS) || have_target_pos))
        return false;
      if (!sub_in.read(YS))
        { KDU_ERROR(e,16); e <<
            KDU_TXT("Malformed Instruction Set (inst) box "
            "found in JPX data source.  Terminated unexpectedly.");
        }
      target_dims.size.x = (int) XS;
      target_dims.size.y = (int) YS;
    }
  else
    target_dims.size = size;

  bool persistent = true;
  kdu_uint32 life=0, next_reuse=0;
  if (have_life_persist)
    {
      if (!(sub_in.read(life) || have_target_pos || have_target_size))
        return false;
      if (!sub_in.read(next_reuse))
        { KDU_ERROR(e,17); e <<
            KDU_TXT("Malformed Instruction Set (inst) box "
            "found in JPX data source.  Terminated unexpectedly.");
        }
      if (life & 0x80000000)
        life &= 0x7FFFFFFF;
      else
        persistent = false;
    }

  if (have_source_region)
    {
      kdu_uint32 XC, YC, WC, HC;
      if (!(sub_in.read(XC) || have_target_pos || have_target_size ||
            have_life_persist))
        return false;
      if (!(sub_in.read(YC) && sub_in.read(WC) && sub_in.read(HC)))
        { KDU_ERROR(e,18); e <<
            KDU_TXT("Malformed Instruction Set (inst) box "
            "found in JPX data source.  Terminated unexpectedly.");
        }
      source_dims.pos.x = (int) XC;
      source_dims.pos.y = (int) YC;
      source_dims.size.x = (int) WC;
      source_dims.size.y = (int) HC;
    }

  if ((tail == NULL) || (tail->duration != 0))
    add_frame();
  jx_instruction *inst = tail->add_instruction((life != 0) || persistent);
  inst->source_dims = source_dims;
  inst->target_dims = target_dims;
  inst->layer_idx = -1; // Evaluate this later on.
  inst->next_reuse = (int) next_reuse;
  max_lookahead--;
  if (inst->next_reuse > max_lookahead)
    max_lookahead = inst->next_reuse;
  tail->duration = (int)(life*tick);
  tail->persistent = persistent;
  return true;
}

/*****************************************************************************/
/*                    jx_composition::assign_layer_indices                   */
/*****************************************************************************/

void
  jx_composition::assign_layer_indices()
{
  jx_frame *fp, *fpp;
  jx_instruction *ip, *ipp;
  int inum, reuse, layer_idx=0;
  for (fp=head; fp != NULL; fp=fp->next)
    for (inum=0, ip=fp->head; ip != NULL; ip=ip->next, inum++)
      {
        if (ip->layer_idx < 0)
          ip->layer_idx = layer_idx++;
        if ((reuse=ip->next_reuse) > 0)
          {
            for (ipp=ip, fpp=fp; reuse > 0; reuse--)
              {
                ipp = ipp->next;
                if (ipp == NULL)
                  {
                    if ((fpp->repeat_count > 0) && (fp != fpp))
                      { // Reusing across a repeated sequence of frames
                        reuse -= fpp->repeat_count*fpp->num_instructions;
                        if (reuse <= 0)
                          { KDU_ERROR(e,19); e <<
                              KDU_TXT("Illegal re-use "
                              "count found in a compositing instruction "
                              "within the JPX composition box.  The specified "
                              "re-use counts found in the box lead to "
                              "multiple conflicting definitions for the "
                              "compositing layer which should be used by a "
                              "particular instruction.");
                          }
                      }
                    fpp=fpp->next;
                    if (fpp == NULL)
                      break;
                    ipp = fpp->head;
                  }
              }
            if ((ipp != NULL) && (reuse == 0))
              ipp->layer_idx = ip->layer_idx;
          }
      }
}

/*****************************************************************************/
/*               jx_composition::remove_invisible_instructions               */
/*****************************************************************************/

void
  jx_composition::remove_invisible_instructions()
{
  jx_frame *fp, *fnext;
  for (fp=head; fp != NULL; fp=fnext)
    {
      fnext = fp->next;

      jx_instruction *ip, *inext;
      for (ip=fp->head; ip != NULL; ip=inext)
        {
          inext = ip->next;
          if (ip->visible)
            continue;

          fp->num_instructions--;
          if (ip->prev == NULL)
            {
              assert(fp->head == ip);
              fp->head = inext;
            }
          else
            ip->prev->next = inext;
          if (inext == NULL)
            {
              assert(fp->tail == ip);
              fp->tail = ip->prev;
            }
          else
            inext->prev = ip->prev;
          delete ip;
        }

      if (fp->head == NULL)
        { // Empty frame detected
          assert(fp->num_instructions == 0);
          if (fp->prev == NULL)
            {
              assert(head == fp);
              head = fnext;
            }
          else
            {
              fp->prev->next = fnext;
              fp->prev->duration += fp->duration;
            }
          if (fnext == NULL)
            {
              assert(tail == fp);
              tail = fp->prev;
            }
          else
            fnext->prev = fp->prev;
          delete fp;
        }
    }
}

/*****************************************************************************/
/*                         jx_composition::finalize                          */
/*****************************************************************************/

void
  jx_composition::finalize(jx_target *owner)
{
  if (is_complete)
    return;
  is_complete = true;

  if (head == NULL)
    return;

  // Start by going through all frames and instructions, verifying that each
  // frame has at least one instruction, expanding repeated frames, and
  // terminating the sequence once we find the first frame which uses
  // a non-existent compositing layer.
  int num_layers = owner->get_num_layers();
  jx_frame *fp;
  jx_instruction *ip;
  for (fp=head; fp != NULL; fp=fp->next)
    {
      if (fp->head == NULL)
        { KDU_ERROR_DEV(e,20); e <<
            KDU_TXT("You must add at least one compositing "
            "instruction to every frame created using "
            "`jpx_composition::add_frame'.");
        }
      if (fp->repeat_count != 0)
        { // Split off the first frame from the rest of the repeating sequence
          jx_frame *new_fp = new jx_frame;
          new_fp->persistent = fp->persistent;
          new_fp->duration = fp->duration;
          if (fp->repeat_count < 0)
            new_fp->repeat_count = -1;
          else
            new_fp->repeat_count = fp->repeat_count-1;
          fp->repeat_count = 0;
          jx_instruction *new_ip;
          for (ip=fp->head; ip != NULL; ip=ip->next)
            {
              assert(ip->visible);
              new_ip = new_fp->add_instruction(true);
              new_ip->layer_idx = ip->layer_idx + ip->increment;
              new_ip->increment = ip->increment;
              new_ip->source_dims = ip->source_dims;
              new_ip->target_dims = ip->target_dims;
            }
          new_fp->next = fp->next;
          new_fp->prev = fp;
          fp->next = new_fp;
          if (new_fp->next != NULL)
            new_fp->next->prev = new_fp;
          else
            {
              assert(fp == tail);
              tail = new_fp;
            }
        }
      bool have_invalid_layer = false;
      for (ip=fp->head; ip != NULL; ip=ip->next)
        {
          ip->next_reuse = 0; // Just in case not properly initialized
          if ((ip->layer_idx >= num_layers) || (ip->layer_idx < 0))
            have_invalid_layer = true;
          kdu_coords lim = ip->target_dims.pos + ip->target_dims.size;
          if (lim.x > size.x)
            size.x = lim.x;
          if (lim.y > size.y)
            size.y = lim.y;
        }
      if (have_invalid_layer && (fp != head))
        break;
    }

  if (fp != NULL)
    { // Delete this and all subsequent layers; can't play them
      tail = fp->prev;
      assert(tail != NULL);
      while ((fp=tail->next) != NULL)
        {
          tail->next = fp->next;
          delete fp;
        }
    }

  // Now introduce invisibles so as to get the layer sequence right
  // At this point, we will set `first_use' to true whenever we encounter
  // an instruction which uses a compositing layer for the first time.
  // This will help make the ensuing algorithm for finding the `next_reuse'
  // links more efficient.
  int layer_idx=0;
  for (fp=head; fp != NULL; fp=fp->next)
    {
      for (ip=fp->head; ip != NULL; ip=ip->next)
        {
          while (ip->layer_idx > layer_idx)
            {
              jx_instruction *new_ip = new jx_instruction;
              new_ip->visible = false;
              new_ip->layer_idx = layer_idx++;
              new_ip->first_use = true;
              new_ip->next = ip;
              new_ip->prev = ip->prev;
              ip->prev = new_ip;
              if (ip == fp->head)
                {
                  fp->head = new_ip;
                  assert(new_ip->prev == NULL);
                }
              else
                new_ip->prev->next = new_ip;
              fp->num_instructions++;
            }
          if (ip->layer_idx == layer_idx)
            {
              ip->first_use = true;
              layer_idx++;
            }
        }
    }

  // Finally, figure out the complete set of `next_reuse' links by walking
  // backwards through the entire sequence of instructions
  jx_frame *fscan;
  jx_instruction *iscan;
  int igap;
  for (fp=tail; fp != NULL; fp=fp->prev)
    {
      for (ip=fp->tail; ip != NULL; ip=ip->prev)
        {
          if (ip->first_use)
            continue;
          iscan = ip->prev;
          fscan = fp;
          igap = 1;
          while ((iscan == NULL) || (iscan->layer_idx != ip->layer_idx))
            {
              if (iscan == NULL)
                {
                  fscan = fscan->prev;
                  if (fscan == NULL)
                    break;
                  iscan = fscan->tail;
                }
              else
                {
                  iscan = iscan->prev;
                  igap++;
                }
            }
          assert(iscan != NULL); // Should not be possible if above algorithm
                     // for assigning invisible instructions worked correctly
          iscan->next_reuse = igap;
        }
    }
}

/*****************************************************************************/
/*                    jx_composition::adjust_compatibility                   */
/*****************************************************************************/

void
  jx_composition::adjust_compatibility(jx_compatibility *compatibility,
                                       jx_target *owner)
{
  if (!is_complete)
    finalize(owner);

  jx_frame *fp;
  jx_instruction *ip;

  if ((head == NULL) || (head->head == head->tail))
    compatibility->add_standard_feature(JPX_SF_COMPOSITING_NOT_REQUIRED);
  int num_layers = owner->get_num_layers();
  if (head == NULL)
    {
      if (num_layers > 1)
        compatibility->add_standard_feature(
          JPX_SF_MULTIPLE_LAYERS_NO_COMPOSITING_OR_ANIMATION);
      return;
    }
  compatibility->add_standard_feature(JPX_SF_COMPOSITING_USED);
  if (head == tail)
    compatibility->add_standard_feature(JPX_SF_NO_ANIMATION);
  else
    { // Check first layer coverage, layer reuse and frame persistence
      bool first_layer_covers = true;
      bool layers_reused = false;
      bool frames_all_persistent = true;
      for (fp=head; fp != NULL; fp=fp->next)
        {
          if ((fp->head->layer_idx != 0) ||
              (fp->head->target_dims.pos.x != 0) ||
              (fp->head->target_dims.pos.y != 0) ||
              (fp->head->target_dims.size != size))
            first_layer_covers = false;
          for (ip=fp->head; ip != NULL; ip=ip->next)
            if (ip->next_reuse > 0)
              layers_reused = true;
          if (!fp->persistent)
            frames_all_persistent = false;
        }
      if (first_layer_covers)
        compatibility->add_standard_feature(
          JPX_SF_ANIMATED_COVERED_BY_FIRST_LAYER);
      else
        compatibility->add_standard_feature(
          JPX_SF_ANIMATED_NOT_COVERED_BY_FIRST_LAYER);
      if (layers_reused)
        compatibility->add_standard_feature(JPX_SF_ANIMATED_LAYERS_REUSED);
      else
        compatibility->add_standard_feature(JPX_SF_ANIMATED_LAYERS_NOT_REUSED);
      if (frames_all_persistent)
        compatibility->add_standard_feature(
          JPX_SF_ANIMATED_PERSISTENT_FRAMES);
      else
        compatibility->add_standard_feature(
          JPX_SF_ANIMATED_NON_PERSISTENT_FRAMES);
    }

  // Check for scaling
  bool scaling_between_layers=false;
  for (fp=head; fp != NULL; fp=fp->next)
    for (ip=fp->head; ip != NULL; ip=ip->next)
      if (ip->source_dims.size != ip->target_dims.size)
        scaling_between_layers = true;
  if (scaling_between_layers)
    compatibility->add_standard_feature(JPX_SF_SCALING_BETWEEN_LAYERS);
}

/*****************************************************************************/
/*                          jx_composition::save_box                         */
/*****************************************************************************/

void
  jx_composition::save_box(jx_target *owner)
{
  if (!is_complete)
    finalize(owner);

  if (head == NULL)
    return; // No composition information provided; do not write the box.

  owner->open_top_box(&comp_out,jp2_composition_4cc);
  jp2_output_box sub;

  sub.open(&comp_out,jp2_comp_options_4cc);
  kdu_uint32 height = (kdu_uint32) size.y;
  kdu_uint32 width = (kdu_uint32) size.x;
  kdu_byte loop = (kdu_byte)(loop_count-1);
  sub.write(height);
  sub.write(width);
  sub.write(loop);
  sub.close();

  jx_frame *fp, *fnext;
  jx_instruction *ip;
  int iset_idx=0;
  for (fp=head; fp != NULL; fp=fnext, iset_idx++)
    {
      sub.open(&comp_out,jp2_comp_instruction_set_4cc);
      kdu_uint16 flags = 39; // Always record all the fields
      kdu_uint32 tick = 1; // All other values are utterly unnecessary!
      kdu_uint16 rept = 0;

      // Calculate number of repetitions by scanning future frames for
      // similarities
      for (fnext=fp->next; fnext != NULL; fnext=fnext->next, rept++)
        {
          if ((fnext->duration != fp->duration) ||
              (fnext->persistent != fp->persistent) ||
              (fnext->num_instructions != fp->num_instructions))
            break;
          jx_instruction *ip2;
          for (ip=fp->head, ip2=fnext->head;
               (ip != NULL) && (ip2 != NULL);
               ip=ip->next, ip2=ip2->next)
            if ((ip->next_reuse != ip2->next_reuse) ||
                (ip->visible != ip2->visible) ||
                (ip->source_dims != ip2->source_dims) ||
                (ip->target_dims != ip2->target_dims) ||
                !ip2->first_use)
              break;
          if ((ip != NULL) || (ip2 != NULL))
            break; // Frame is not compatible with previous one
        }
      sub.write(flags);
      sub.write(rept);
      sub.write(tick);
      int inum_idx = 0;
      for (ip=fp->head; ip != NULL; ip=ip->next)
        { // Record instruction
          kdu_uint32 life = 0;
          ip->iset_idx = iset_idx;
          ip->inum_idx = inum_idx++;
          if (ip != fp->tail)
            { // Non-terminal instructions in frame have duration=0
              if (ip->visible)
                life |= 0x80000000; // Add `persists' flag
            }
          else
            { // Terminate frame
              if (fp->persistent)
                {
                  life |= 0x80000000;
                  assert(ip->visible); // Can't have invisibles at end of
                                       // persistent frame
                }
              life |= ((kdu_uint32) fp->duration) & 0x7FFFFFFF;
            }
          sub.write((kdu_uint32) ip->target_dims.pos.x);
          sub.write((kdu_uint32) ip->target_dims.pos.y);
          sub.write((kdu_uint32) ip->target_dims.size.x);
          sub.write((kdu_uint32) ip->target_dims.size.y);
          sub.write(life);
          sub.write((kdu_uint32) ip->next_reuse);
          sub.write((kdu_uint32) ip->source_dims.pos.x);
          sub.write((kdu_uint32) ip->source_dims.pos.y);
          sub.write((kdu_uint32) ip->source_dims.size.x);
          sub.write((kdu_uint32) ip->source_dims.size.y);
        }
      sub.close();
    }

  comp_out.close();
}


/* ========================================================================= */
/*                             jpx_composition                               */
/* ========================================================================= */

/*****************************************************************************/
/*                      jpx_composition::get_global_info                     */
/*****************************************************************************/

int
  jpx_composition::get_global_info(kdu_coords &size)
{
  if (state == NULL)
    return -1;
  size = state->size;
  return state->loop_count;
}

/*****************************************************************************/
/*                      jpx_composition::get_next_frame                      */
/*****************************************************************************/

jx_frame *
  jpx_composition::get_next_frame(jx_frame *last_frame)
{
  if (state == NULL)
    return NULL;
  if (last_frame == NULL)
    return state->head;
  else
    return last_frame->next;
}

/*****************************************************************************/
/*                      jpx_composition::get_prev_frame                      */
/*****************************************************************************/

jx_frame *
  jpx_composition::get_prev_frame(jx_frame *last_frame)
{
  if (state == NULL)
    return NULL;
  if (last_frame == NULL)
    return NULL;
  return last_frame->prev;
}

/*****************************************************************************/
/*                      jpx_composition::get_frame_info                      */
/*****************************************************************************/

void
  jpx_composition::get_frame_info(jx_frame *frame_ref, int &num_instructions,
                                  int &duration, int &repeat_count,
                                  bool &is_persistent)
{
  assert(state != NULL);
  num_instructions = frame_ref->num_instructions;
  duration = frame_ref->duration;
  repeat_count = frame_ref->repeat_count;
  is_persistent = frame_ref->persistent;
}

/*****************************************************************************/
/*                 jpx_composition::get_last_persistent_frame                */
/*****************************************************************************/

jx_frame *
  jpx_composition::get_last_persistent_frame(jx_frame *frame_ref)
{
  if (state == NULL)
    return NULL;
  return frame_ref->last_persistent_frame;
}

/*****************************************************************************/
/*                      jpx_composition::get_instruction                     */
/*****************************************************************************/

bool
  jpx_composition::get_instruction(jx_frame *frame, int instruction_idx,
                                   int &layer_idx, int &increment,
                                   bool &is_reused, kdu_dims &source_dims,
                                   kdu_dims &target_dims)
{
  if (state == NULL)
    return false;
  if (instruction_idx >= frame->num_instructions)
    return false;
  jx_instruction *ip;
  for (ip=frame->head; instruction_idx > 0; instruction_idx--, ip=ip->next)
    assert(ip != NULL);
  layer_idx = ip->layer_idx;
  increment = ip->increment;
  is_reused = (ip->next_reuse != 0);
  source_dims = ip->source_dims;
  target_dims = ip->target_dims;
  return true;
}

/*****************************************************************************/
/*                     jpx_composition::get_original_iset                    */
/*****************************************************************************/

bool
  jpx_composition::get_original_iset(jx_frame *frame, int instruction_idx,
                                     int &iset_idx, int &inum_idx)
{
  if (state == NULL)
    return false;
  if (instruction_idx >= frame->num_instructions)
    return false;
  jx_instruction *ip;
  for (ip=frame->head; instruction_idx > 0; instruction_idx--, ip=ip->next)
    assert(ip != NULL);
  iset_idx = ip->iset_idx;
  inum_idx = ip->inum_idx;
  return true;
}

/*****************************************************************************/
/*                         jpx_composition::add_frame                        */
/*****************************************************************************/

jx_frame *
  jpx_composition::add_frame(int duration, int repeat_count,
                             bool is_persistent)
{
  if (state == NULL)
    return NULL;
  if ((state->tail != NULL) && (state->tail->duration == 0))
    { KDU_ERROR_DEV(e,21); e <<
        KDU_TXT("Attempting to add multiple animation frames which "
        "have a temporal duration of 0 milliseconds, using the "
        "`jpx_composition::add_frame' function.  This is clearly silly.");
    }
  state->add_frame();
  state->tail->duration = duration;
  state->tail->repeat_count = repeat_count;
  state->tail->persistent = is_persistent;
  return state->tail;
}

/*****************************************************************************/
/*                     jpx_composition::add_instruction                      */
/*****************************************************************************/

int
  jpx_composition::add_instruction(jx_frame *frame, int layer_idx,
                                   int increment, kdu_dims source_dims,
                                   kdu_dims target_dims)
{
  if (state == NULL)
    return -1;
  jx_instruction *inst = frame->add_instruction(true);
  inst->layer_idx = layer_idx;
  inst->increment = increment;
  inst->source_dims = source_dims;
  inst->target_dims = target_dims;
  return frame->num_instructions-1;
}

/*****************************************************************************/
/*                     jpx_composition::set_loop_count                       */
/*****************************************************************************/

void
  jpx_composition::set_loop_count(int count)
{
  if ((count < 0) || (count > 255))
    { KDU_ERROR_DEV(e,22); e <<
        KDU_TXT("Illegal loop count supplied to "
        "`jpx_composition::set_loop_count'.  Legal values must be in the "
        "range 0 (indefinite looping) to 255 (max explicit repetitions).");
    }
  state->loop_count = count;
}

/*****************************************************************************/
/*                           jpx_composition::copy                           */
/*****************************************************************************/

void
  jpx_composition::copy(jpx_composition src)
{
  assert((state != NULL) && (src.state != NULL));
  jx_frame *sp, *dp;
  jx_instruction *spi, *dpi;

  for (sp=src.state->head; sp != NULL; sp=sp->next)
    {
      if (sp->head == NULL)
        continue;
      state->add_frame();
      dp = state->tail;
      dp->duration = sp->duration;
      dp->repeat_count = sp->repeat_count;
      dp->persistent = sp->persistent;
      for (spi=sp->head; spi != NULL; spi=spi->next)
        {
          dpi = dp->add_instruction(true);
          dpi->layer_idx = spi->layer_idx;
          dpi->increment = spi->increment;
          dpi->source_dims = spi->source_dims;
          dpi->target_dims = spi->target_dims;
        }
    }
}


/* ========================================================================= */
/*                            jpx_frame_expander                             */
/* ========================================================================= */

/*****************************************************************************/
/*             jpx_frame_expander::test_codestream_visibility                */
/*****************************************************************************/

int
  jpx_frame_expander::test_codestream_visibility(jpx_source *source,
                                                 jx_frame *the_frame,
                                                 int the_iteration_idx,
                                                 bool follow_persistence,
                                                 int codestream_idx,
                                                 kdu_dims &composition_region,
                                                 kdu_dims codestream_roi,
                                                 bool ignore_use_in_alpha,
                                                 int initial_matches_to_skip)
{
  jpx_composition composition = source->access_composition();
  if (!composition)
    return -1;
  kdu_dims comp_dims; composition.get_global_info(comp_dims.size);
  if (composition_region.is_empty())
    composition_region = comp_dims;
  jpx_codestream_source stream =
  source->access_codestream(codestream_idx);
  if (stream.exists())
    {
      jp2_dimensions cs_dims = stream.access_dimensions();
      if (cs_dims.exists())
        {
          kdu_dims codestream_dims;
          codestream_dims.size = cs_dims.get_size();
          if (!codestream_roi.is_empty())
            codestream_roi &= codestream_dims;
          else
            codestream_roi = codestream_dims;
          if (codestream_roi.is_empty())
            return -1;
        }
    }
  
  int max_compositing_layers;
  bool max_layers_known =
    source->count_compositing_layers(max_compositing_layers);

  // In the following, we repeatedly cycle through the frame members (layers)
  // starting from the top-most layer.  We keep track of the frame member
  // in which the codestream was found in the last iteration of the loop,
  // if any, along with the region which it occupies on the composition
  // surface.  This is done via the following two variables.  While examining
  // members which come before (above in the composition sequence) this
  // last member, we determine whether or not they hide it and how much
  // of it they hide.  We examine members beyond the last member in order
  // to find new uses of the codestream, as required by the
  // `initial_matches_to_skip' argument.
  int last_member_idx=-1; // We haven't found any match yet
  int last_layer_idx=-1; // Compositing layer associated with last member
  kdu_dims last_composition_region;
  jx_frame *frame = the_frame; // Used to scan through the iterations
  int iteration_idx = the_iteration_idx;
  int member_idx = 0;
  while (frame != NULL)
    {
      int n, num_instructions, duration, repeat_count;
      bool is_persistent;
      composition.get_frame_info(frame,num_instructions,duration,
                                 repeat_count,is_persistent);
      if (iteration_idx < 0)
        iteration_idx = repeat_count;
      else if (iteration_idx > repeat_count)
        return -1; // Cannot open requested frame
          
      int layer_idx = -1;
      jpx_layer_source layer;
      kdu_dims source_dims, target_dims;
      for (n=num_instructions-1; n >= 0; n--, member_idx++)
        {
          int increment;
          bool is_reused;
          composition.get_instruction(frame,n,layer_idx,increment,
                                      is_reused,source_dims,target_dims);
          layer_idx += increment*iteration_idx;
          if ((layer_idx < 0) ||
              (max_layers_known &&
               (layer_idx >= max_compositing_layers)))
            continue; // Treat this layer as if it does not exist for now
          if (!(layer = source->access_layer(layer_idx,false)).exists())
            continue; // Treat layer as if it does not exist for now
          if (source_dims.is_empty())
            {
              source_dims.pos = kdu_coords();
              source_dims.size = layer.get_layer_size();
            }
          if (target_dims.is_empty())
            target_dims.size = source_dims.size;
          jp2_channels channels = layer.access_channels();
          if (!channels)
            continue; // Treat layer as if it does not exist for now
          int c, num_colours = channels.get_num_colours();
          int comp_idx, lut_idx, str_idx, key_val;
          if ((member_idx < last_member_idx) &&
              !last_composition_region.is_empty())
            { // See if this layer covers `last_composition_region'
              for (c=0; c < num_colours; c++)
                if (channels.get_opacity_mapping(c,comp_idx,lut_idx,
                                                 str_idx) ||
                    channels.get_premult_mapping(c,comp_idx,lut_idx,
                                                 str_idx) ||
                    channels.get_chroma_key(0,key_val))
                  break; // Layer not completely opaque
              if (c == num_colours)
                { // Layer is opaque; let's test for intersection
                  kdu_dims isect = last_composition_region & target_dims;
                  if (isect.size.x == last_composition_region.size.x)
                    {
                      if (isect.pos.y == last_composition_region.size.y)
                        {
                          last_composition_region.pos.y += isect.size.y;
                          last_composition_region.size.y -= isect.size.y;
                        }
                      else if ((isect.pos.y+isect.size.y) ==
                               (last_composition_region.pos.y+
                                last_composition_region.size.y))
                        last_composition_region.size.y -= isect.size.y; 
                    }
                  else if (isect.size.y == last_composition_region.size.y)
                    {
                      if (isect.pos.x == last_composition_region.size.x)
                        {
                          last_composition_region.pos.x += isect.size.x;
                          last_composition_region.size.x -= isect.size.x;
                        }
                      else if ((isect.pos.x+isect.size.x) ==
                               (last_composition_region.pos.x+
                                last_composition_region.size.x))
                        last_composition_region.size.x -= isect.size.x; 
                    }
                }
            }
          else if (member_idx == last_member_idx)
            { // Time to assess whether or not the last member is visible
              if (!last_composition_region.is_empty())
                {
                  if (initial_matches_to_skip == 0)
                    {
                      composition_region = last_composition_region;
                      return last_layer_idx;
                    }
                  else
                    initial_matches_to_skip--;
                }
              last_member_idx = -1;
              last_layer_idx = -1;
              last_composition_region = kdu_dims();
            }
          else if (last_member_idx < 0)
            { // Looking for a new member which contains the codestream
              for (c=0; c < num_colours; c++)
                {
                  if (channels.get_colour_mapping(c,comp_idx,
                                                  lut_idx,str_idx) &&
                      (str_idx == codestream_idx))
                    break;
                  if (ignore_use_in_alpha)
                    continue;
                  if (channels.get_opacity_mapping(c,comp_idx,
                                                   lut_idx,str_idx) &&
                      (str_idx == codestream_idx))
                    break;
                  if (channels.get_premult_mapping(c,comp_idx,
                                                   lut_idx,str_idx) &&
                      (str_idx == codestream_idx))
                    break;
                }
              if (c < num_colours)
                break; // Found matching codestream
            }
        }
              
      if (n >= 0)
        { // Found a new member; set up the `last_composition_region'
          // object and then start another iteration thru the members
          kdu_coords align, sampling, denominator;
          int c, str_idx;
          for (c=0; (str_idx =
                     layer.get_codestream_registration(c,align,sampling,
                                                       denominator)) >= 0;
               c++)
            if (str_idx == codestream_idx)
              break;
          if (str_idx < 0)
            { // Failed to identify the codestream; makes no sense really
              assert(0);
              return -1;
            }
          // Now convert `codestream_roi' into a region on the compositing
          // surface, storing this region in `last_composition_region'
          if (!codestream_roi)
            last_composition_region = target_dims;
          else
            {
              kdu_coords min = codestream_roi.pos;
              kdu_coords lim = min + codestream_roi.size;
              if ((denominator.x > 0) && (denominator.y > 0))
                {
                  min.x = long_floor_ratio(((kdu_long) min.x)*sampling.x,
                                           denominator.x);
                  min.y = long_floor_ratio(((kdu_long) min.y)*sampling.y,
                                           denominator.y);
                  lim.x = long_ceil_ratio(((kdu_long) lim.x)*sampling.x,
                                          denominator.x);
                  lim.y = long_ceil_ratio(((kdu_long) lim.y)*sampling.y,
                                          denominator.y);
                }
              last_composition_region.pos = min;
              last_composition_region.size = lim - min;
              last_composition_region &= source_dims;
              last_composition_region.pos -= source_dims.pos;
              if (target_dims.size != source_dims.size)
                { // Need to further scale the region
                  min = last_composition_region.pos;
                  lim = min + last_composition_region.size;
                  min.x =
                    long_floor_ratio(((kdu_long) min.x)*target_dims.size.x,
                                     source_dims.size.x);
                  min.y =
                    long_floor_ratio(((kdu_long) min.y)*target_dims.size.y,
                                     source_dims.size.y);
                  lim.x =
                    long_ceil_ratio(((kdu_long) lim.x)*target_dims.size.x,
                                    source_dims.size.x);
                  lim.y =
                    long_ceil_ratio(((kdu_long) lim.y)*target_dims.size.x,
                                    source_dims.size.x);
                  composition_region.pos = min;
                  composition_region.size = lim - min;
                }
              last_composition_region.pos += target_dims.pos;
              last_composition_region &= target_dims; // Just to be sure
            }
          last_member_idx = member_idx;
          last_layer_idx = layer_idx;
          member_idx = 0;
          frame = the_frame;
          iteration_idx = the_iteration_idx;
          continue;
        }
                    
      // See if there are more frames which must be composed with this one
      iteration_idx--;
      if ((frame != NULL) && ((iteration_idx < 0) || !is_persistent))
        {
          frame = composition.get_last_persistent_frame(frame);
          iteration_idx = -1; // Find last iteration at start of loop
        }
    }
  return -1; // Didn't find a match
}

/*****************************************************************************/
/*                      jpx_frame_expander::construct                        */
/*****************************************************************************/

bool
  jpx_frame_expander::construct(jpx_source *source, jx_frame *frame,
                                int iteration_idx, bool follow_persistence,
                                kdu_dims region_of_interest)
{
  non_covering_members = false;
  num_members = 0;
  jpx_composition composition = source->access_composition();
  if (!composition)
    return false;
  kdu_dims comp_dims; composition.get_global_info(comp_dims.size);
  if (region_of_interest.is_empty())
    region_of_interest = comp_dims;

  kdu_dims opaque_dims;
  kdu_dims opaque_roi; // Amount of region of interest which is opaque
  int max_compositing_layers;
  bool max_layers_known =
    source->count_compositing_layers(max_compositing_layers);
  while (frame != NULL)
    {
      int n, num_instructions, duration, repeat_count;
      bool is_persistent;
      composition.get_frame_info(frame,num_instructions,duration,
                                 repeat_count,is_persistent);
      if (iteration_idx < 0)
        iteration_idx = repeat_count;
      else if (iteration_idx > repeat_count)
        { // Cannot open requested frame
          num_members = 0;
          return true;
        }
      for (n=num_instructions-1; n >= 0; n--)
        {
          kdu_dims source_dims, target_dims;
          int layer_idx, increment;
          bool is_reused;
          composition.get_instruction(frame,n,layer_idx,increment,
                                      is_reused,source_dims,target_dims);

          // Calculate layer index and open the `jpx_layer_source' if possible
          layer_idx += increment*iteration_idx;
          if ((layer_idx < 0) ||
              (max_layers_known && (layer_idx >= max_compositing_layers)))
            { // Cannot open frame
              num_members = 0;
              return true;
            }

          jpx_layer_source layer = source->access_layer(layer_idx,false);
          if (source_dims.is_empty() && layer.exists())
            source_dims.size = layer.get_layer_size();
          if (target_dims.is_empty())
            target_dims.size = source_dims.size;
          kdu_dims test_dims = target_dims & comp_dims;
          kdu_dims test_roi = target_dims & region_of_interest;
          if ((!target_dims.is_empty()) &&
              ((opaque_dims & test_dims) == test_dims))
              continue; // Globally invisible

          // Add a new member
          if (max_members == num_members)
            {
              int new_max_members = max_members + num_members+8;
              jx_frame_member *tmp = new jx_frame_member[new_max_members];
              for (int m=0; m < num_members; m++)
                tmp[m] = members[m];
              if (members != NULL)
                delete[] members;
              max_members = new_max_members;
              members = tmp;
            }
          jx_frame_member *mem = members + (num_members++);
          mem->frame = frame;
          mem->instruction_idx = n;
          mem->layer_idx = layer_idx;
          mem->source_dims = source_dims;
          mem->target_dims = target_dims;
          mem->covers_composition = (target_dims == comp_dims);
          mem->layer_is_accessible = layer.exists();
          mem->may_be_visible_under_roi =
            target_dims.is_empty() ||
            ((!test_roi.is_empty()) && ((test_roi & opaque_roi) != test_roi));
          mem->is_opaque = false; // Until proven otherwise
          if (layer.exists())
            { // Determine opacity of layer
              jp2_channels channels = layer.access_channels();
              int comp_idx, lut_idx, str_idx;
              kdu_int32 key_val;
              mem->is_opaque =
                !(channels.get_opacity_mapping(0,comp_idx,lut_idx,str_idx) ||
                  channels.get_premult_mapping(0,comp_idx,lut_idx,str_idx) ||
                  channels.get_chroma_key(0,key_val));
            }

          // Update the opaque regions and see if whole composition is covered
          if (mem->is_opaque)
            {
              if (test_dims.area() > opaque_dims.area())
                opaque_dims = test_dims;
              if (test_roi.area() > opaque_roi.area())
                opaque_roi = test_roi;
              if (opaque_dims == comp_dims)
                { // Can stop looking for more instructions
                  frame = NULL;
                  break;
                }
            }
        }
      
      // See if there are any more frames which must be composed with this one
      iteration_idx--;
      if ((frame != NULL) && ((iteration_idx < 0) || !is_persistent))
        {
          frame = composition.get_last_persistent_frame(frame);
          iteration_idx = -1; // Find last iteration at start of loop
        }
    }

  // Now go back over things to see if some extra members can be removed
  int n, m;
  bool can_access_all_members = true;
  for (n=0; n < num_members; n++)
    {
      if (!members[n].may_be_visible_under_roi)
        {
          if (!members[n].layer_is_accessible)
            { // May still need to include this member, unless included already
              int layer_idx = members[n].layer_idx;
              for (m=0; m < num_members; m++)
                if ((members[m].layer_idx == layer_idx) &&
                    ((m < n) || members[m].may_be_visible_under_roi))
                  { // We can eliminate the member instruction
                    members[n].layer_is_accessible = true; // Forces removal
                    break;
                  }
            }
          if (members[n].layer_is_accessible)
            {
              num_members--;
              for (m=n; m < num_members; m++)
                members[m] = members[m+1];
              n--; // So that we consider the next member correctly
              continue;
            }
        }
      if (!members[n].layer_is_accessible)
        can_access_all_members = false;
      else if (!members[n].covers_composition)
        non_covering_members = true;
    }

  return can_access_all_members;
}


/* ========================================================================= */
/*                                jx_numlist                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                          jx_numlist::~jx_numlist                          */
/*****************************************************************************/

jx_numlist::~jx_numlist()
{
  if ((codestream_indices != NULL) &&
      (codestream_indices != &single_codestream_idx))
    delete[] codestream_indices;
  if ((layer_indices != NULL) && (layer_indices != &single_layer_idx))
    delete[] layer_indices;
}

/*****************************************************************************/
/*                        jx_numlist::add_codestream                         */
/*****************************************************************************/

void
  jx_numlist::add_codestream(int idx)
{
  int n;
  for (n=0; n < num_codestreams; n++)
    if (codestream_indices[n] == idx)
      return;
  if (num_codestreams == 0)
    {
      assert(codestream_indices == NULL);
      max_codestreams = 1;
      codestream_indices = &single_codestream_idx;
    }
  else if (num_codestreams >= max_codestreams)
    { // Allocate appropriate array
      int new_max_codestreams = max_codestreams + 8;
      int *tmp = new int[new_max_codestreams];
      for (n=0; n < num_codestreams; n++)
        tmp[n] = codestream_indices[n];
      if (codestream_indices != &single_codestream_idx)
        delete[] codestream_indices;
      codestream_indices = tmp;
      max_codestreams = new_max_codestreams;
    }
  num_codestreams++;
  codestream_indices[n] = idx;
}

/*****************************************************************************/
/*                    jx_numlist::add_compositing_layer                      */
/*****************************************************************************/

void
  jx_numlist::add_compositing_layer(int idx)
{
  int n;
  for (n=0; n < num_compositing_layers; n++)
    if (layer_indices[n] == idx)
      return;
  if (num_compositing_layers == 0)
    {
      assert(layer_indices == NULL);
      max_compositing_layers = 1;
      layer_indices = &single_layer_idx;
    }
  else if (num_compositing_layers >= max_compositing_layers)
    { // Allocate appropriate array
      int new_max_layers = max_compositing_layers + 8;
      int *tmp = new int[new_max_layers];
      for (n=0; n < num_compositing_layers; n++)
        tmp[n] = layer_indices[n];
      if (layer_indices != &single_layer_idx)
        delete[] layer_indices;
      layer_indices = tmp;
      max_compositing_layers = new_max_layers;
    }
  num_compositing_layers++;
  layer_indices[n] = idx;
}

/*****************************************************************************/
/*                            jx_numlist::write                              */
/*****************************************************************************/

void
  jx_numlist::write(jp2_output_box &box)
{
  int n;
  for (n=0; n < num_codestreams; n++)
    box.write((kdu_uint32)(codestream_indices[n] | 0x01000000));
  for (n=0; n < num_compositing_layers; n++)
    box.write((kdu_uint32)(layer_indices[n] | 0x02000000));
  if (rendered_result)
    box.write((kdu_uint32) 0x00000000);
}


/* ========================================================================= */
/*                                 jpx_roi                                   */
/* ========================================================================= */

/*****************************************************************************/
/*                        jpx_roi::init_quadrilateral                        */
/*****************************************************************************/

void
  jpx_roi::init_quadrilateral(kdu_coords v1, kdu_coords v2,
                              kdu_coords v3, kdu_coords v4,
                              bool coded, kdu_byte priority)
{
  is_elliptical = false; flags = JPX_QUADRILATERAL_ROI;
  is_encoded = coded; coding_priority = priority;
  elliptical_skew.x = elliptical_skew.y = 0;
  vertices[0]=v1; vertices[1]=v2; vertices[2]=v3; vertices[3]=v4;
  int v, top_v=0, min_x=v1.x, max_x=v1.x, min_y=v1.y, max_y=v1.y;
  for (v=1; v < 4; v++)
    {
      if (vertices[v].x < min_x)
        min_x = vertices[v].x;
      else if (vertices[v].x > max_x)
        max_x = vertices[v].x;
      if (vertices[v].y < min_y)
        { min_y = vertices[v].y; top_v = v; }
      else if (vertices[v].y > max_y)
        max_y = vertices[v].y;
    }
  while (top_v > 0)
    {
      v1 = vertices[0];
      for (v=0; v < 3; v++)
        vertices[v] = vertices[v+1];
      vertices[3] = v1;
      top_v--;
    }
  region.pos.x = min_x;  region.size.x = max_x + 1 - min_x;
  region.pos.y = min_y;  region.size.y = max_y + 1 - min_y;
  if ((vertices[0].x == min_x) && (vertices[0].y == min_y) &&
      (vertices[1].x == max_x) && (vertices[1].y == min_y) &&
      (vertices[2].x == max_x) && (vertices[2].y == max_y) &&
      (vertices[3].x == min_x) && (vertices[3].y == max_y))
    flags = 0; // Region is a simple rectangle
}

/*****************************************************************************/
/*                          jpx_roi::init_ellipse                            */
/*****************************************************************************/

void
  jpx_roi::init_ellipse(kdu_coords centre, kdu_coords extent,
                        kdu_coords skew, bool coded, kdu_byte priority)
{
  if (extent.x < 1) extent.x = 1;
  if (extent.y < 1) extent.y = 1;
  is_elliptical = true; flags = 0;  
  is_encoded = coded; coding_priority = priority;
  region.size.x = (extent.x<<1)+1;
  region.size.y = (extent.y<<1)+1;
  region.pos.x = centre.x - extent.x;
  region.pos.y = centre.y - extent.y;
  elliptical_skew = skew;
  double gamma;
  if (!compute_gamma_and_extent(gamma,extent))
    { // Need to regenerate compatible skew values
      skew.x = (int) floor(0.5+gamma*extent.x);
      skew.y = (int) floor(0.5+gamma*extent.y);
    }
  if (skew.x <= -extent.x) skew.x = 1-extent.x;
  if (skew.x >= extent.x) skew.x = extent.x-1;
  if (skew.y <= -extent.y) skew.y = 1-extent.y;
  if (skew.y >= extent.y) skew.y = extent.y-1;
  elliptical_skew = skew;
}

/*****************************************************************************/
/*                    jpx_roi::compute_gamma_and_extent                      */
/*****************************************************************************/

bool
  jpx_roi::compute_gamma_and_extent(double &gamma, kdu_coords &extent) const
{
  assert(is_elliptical);
  kdu_coords skew = elliptical_skew;
  extent.x = region.size.x>>1;
  extent.y = region.size.y>>1;
  if (extent.x < 1) extent.x = 1;
  if (extent.y < 1) extent.y = 1;
  double inv_Wo = 1.0/extent.x, inv_Ho = 1.0/extent.y;
  
     /* Discussion:
        We should have `skew.x' and `skew.y' within +/-0.5 of Sx and Sy, where
        Sx=gamma*Wo and Sy=gamma*Ho.  Suppose that this is true.  Then
        gamma must lie in the intervals (skew.x-0.5)/Wo to (skew.x+0.5)/Wo
        and (skew.y-0.5)/Ho to (skew.y+0.5)/Ho. If these intervals
        intersect, we take gamma to be the value in the mid-point of the
        intersection. */

  double lb1 = (skew.y-0.5)*inv_Ho; // One lower bound
  double lb2 = (skew.x-0.5)*inv_Wo; // The other lower bound
  double ub1 = (skew.y+0.5)*inv_Ho; // One upper bound
  double ub2 = (skew.x+0.5)*inv_Wo; // The other upper bound
  double lb = (lb1>lb2)?lb1:lb2;
  double ub = (ub1<ub2)?ub1:ub2;
  if ((lb <= (ub+0.0001)) &&
      (skew.x > -extent.x) && (skew.x < extent.x) &&
      (skew.y > -extent.y) && (skew.y < extent.y))
    { 
      gamma = (lb+ub)*0.5;
      return true;
    }
  
  // If we get here, we have to synthesize a gamma value from the inconsistent
  // `skew.x' and `skew.y' values.  We do this in such a way that the
  // synthesized gamma can be multiplied by Ho and Wo and rounded to the
  // nearest integer so as to derive `skew.x' and `skew.y' values which are
  // both consistent and legal.  This means that the derived `skew.x' and
  // `skew.y' values using this method must have absolute values which are
  // smaller than `extent.x' and `extent.y', respectively.
  ub1 = (extent.x+0.4)*inv_Wo; // So rounding stays within skew_range.x
  ub2 = (extent.y+0.4)*inv_Ho; // So rounding stays within skew_range.y
  ub = (ub1 < ub2)?ub1:ub2;
  assert(ub >= 0.0);
  if ((skew.x ^ skew.y) < 0)
    gamma = 0.0; // Incompatible signs!!
  else
    gamma = sqrt((skew.x*inv_Wo) * (skew.y*inv_Ho));
  if (gamma > ub)
    gamma = ub;
  assert(gamma < 1.0);
  if ((skew.x+skew.y) < 0)
    gamma = -gamma;
  return false;
}

/*****************************************************************************/
/*                          jpx_roi::init_ellipse                            */
/*****************************************************************************/

void
  jpx_roi::init_ellipse(kdu_coords centre, const double axis_extents[2],
                        double tan_theta, bool coded, kdu_byte priority)
{
  // Here is how the algorithm here works:
  //    First, we recognize that the boundary of the initial cardinally aligned
  // ellipse is described by:
  //       x = U * sin(phi), y = V * cos(phi)
  // where (x,y) is the displacement of a boundary point from the centre, phi
  // is an angular parameter, and V=axis_extents[0] and U=axis_extents[1].
  //    Next, we recognize that after rotation of the ellipse, the boundary
  // points are given by:
  //       xr = x*cos(theta)-y*sin(theta);  yr = y*cos(theta)+x*sin(theta).
  // Then the top/bottom boundaries of the oriented ellipse's bounding box are
  // encountered at parameter phi for which (d yr)/(d phi) = 0.  Now some
  // elementary maths yields
  //       tan(phi) = (U/V) * tan(theta).
  // Thence, we find that
  //       Ho = +/-yr(phi) = V*cos(phi)cos(theta) + U*sin(phi)sin(theta)
  //       skew.x = +/-xr(phi) = V*cos(phi)sin(theta) - U*sin(phi)cos(theta)
  //                           = U*sin(theta)cos(phi)*[V/U - U/V]
  // The left/right boundaries of the oriented ellipse's bounding box are
  // encountered at parameter phi for which (d xr)/(d phi) = 0.  Write this
  // value of phi as psi (to avoid confusion).  Then some elementary maths
  // yields
  //       cot(psi) = -(V/U) * tan(theta) = -(V^2 / U^2) tan(phi)
  // It is convenient to define zeta = psi +/- pi/2 such that |zeta| < pi
  // and
  //       tan(zeta) = -cot(psi).
  // Then
  //       tan(zeta) = V/U * tan(theta) = (V^2 / U^2) tan(phi)
  // Thence, we find that
  //       Wo = +/-xr(psi) = U*cos(zeta)cos(theta) + V*sin(zeta)sin(theta)
  // and
  //       skew.y = +/-yr(psi) = V*sin(zeta)*cos(theta)-U*cos(zeta)*sin(theta)
  //                           = V*sin(theta)cos(zeta)*[V/U - U/V]
  // In the above, we have chosen the signs carefully so as to satisfy the
  // requirements that Ho and Wo must be positive, and skew.x and skew.y
  // must each have the same sign as theta if U < V and the opposite sign to
  // theta if U > V.
  //   The above equations can actually be simplified to the following:
  //       Ho = V*cos(theta)/cos(phi)
  //       Wo = U*cos(theta)/cos(zeta)
  // with
  //       tan(phi) = U/V * tan(theta)    and     tan(zeta) = V/U * tan(theta)
  // and
  //       skew.x = Wo * gamma     and     skew.y = Ho * gamma
  // with
  //       gamma = tan(theta) * cos(phi) * cos(zeta) * [V/U - U/V]
  
  double V = axis_extents[0], U = axis_extents[1];
  if (U < 0.25) U = 0.25;
  if (V < 0.25) V = 0.25;
  if (tan_theta < -1.0)
    tan_theta = -1.0;
  else if (tan_theta > 1.0)
    tan_theta = 1.0;
  double tan_phi  = U * tan_theta / V;
  double tan_zeta = V * tan_theta / U;
  double cos_phi = sqrt(1.0 / (1.0 + tan_phi*tan_phi));
  double cos_zeta = sqrt(1.0 / (1.0 + tan_zeta*tan_zeta));
  double cos_theta = sqrt(1.0 / (1.0 + tan_theta*tan_theta));
  double Ho = V*cos_theta/cos_phi;
  double Wo = U*cos_theta/cos_zeta;
  double gamma = tan_theta * cos_phi * cos_zeta * (V/U - U/V);
  double skew_x = Wo * gamma;
  double skew_y = Ho * gamma;
  
  kdu_coords extent, skew;
  extent.x = (int) floor(Wo+0.5);
  extent.y = (int) floor(Ho+0.5);
  skew.x = (int) floor(skew_x+0.5);
  skew.y = (int) floor(skew_y+0.5);
  init_ellipse(centre,extent,skew,coded,priority);
}

/*****************************************************************************/
/*                          jpx_roi::get_ellipse                             */
/*****************************************************************************/

bool
  jpx_roi::get_ellipse(kdu_coords &centre, double axis_extents[2],
                       double &tan_theta) const
{
  if (!is_elliptical) return false;

  // This function essentially inverts the mathematical process used by
  // the second form of the `init_ellipse' function.  The inversion is
  // not so straightforward, but the following results hold:
  //    Let R=Wo/Ho be the known ratio between the width and the height of
  // the bounding box of the oriented ellipse.  Also, let rho=U/V be the
  // (initially unknown) ratio between the width and the height of the
  // cardinally aligned ellipse prior to clockwise rotation through angle
  // theta.  Also, let gamma be the skewing parameter, whose value is
  // derived in the `init_ellipse' function to be:
  //         gamma = tan(theta) * cos(phi) * cos(zeta) * [1/rho - rho]
  // where
  //         tan(phi) = rho * tan(theta)     and
  //         tan(zeta) = 1/rho * tan(theta).
  // The values of Wo, Ho and gamma are recovered by calling the
  // internal helper function, `compute_gamma_and_extent'.  Using the
  // relationships deried above for the second `init_ellipse' function, it
  // is known that
  //          R = rho * cos(phi) / cos(zeta).
  // Thus far, things are quite simple.  After quite some manipulation, it
  // can be shown that the solution for rho and tan(theta) is given by
  //          rho = sqrt[Q +/- sqrt(Q^2 - 1)]
  //          tan(theta) = (R^2 - rho^2) / ((1 + rho^2) * R * gamma)
  // where
  //          Q = (R^4 + 2*gamma^2*R^2 + 1) / (2*R^2*(1-gamma^2))
  // Moreover, the two solutions for Q (due to the +/- terms in the above
  // equation) correspond to reciprocal values for rho (in which U is
  // interchanged with V) and values of theta which are separated by pi/2.
  // In other words, the two solutions for Q simply correspond to different
  // interpretations concerning which of the two elliptical axes is initially
  // considered to lie in the horizontal direction (prior to clockwise
  // rotation).  According to the definition of the present function, we
  // pick the solution for which -pi/4 <= theta <= pi/4, i.e., for which
  // -1 <= tan(theta) <= 1.
  //    Note that once we have tan(theta) and rho, we can compute
  //           tan(phi) = rho * tan(theta)
  //           tan(zeta) = 1/rho * tan(theta)
  //           axis_extents[0] = Ho * cos(phi)/cos(theta)  and
  //           axis_extents[1] = Wo * cos(zeta)/cos(theta).
  
  centre.x = region.pos.x + (region.size.x>>1);
  centre.y = region.pos.y + (region.size.y>>1);
  kdu_coords extent;
  double gamma;
  compute_gamma_and_extent(gamma,extent);
  double gamma_sq = gamma*gamma;
  if ((elliptical_skew == kdu_coords()) || (gamma_sq <= 0.0))
    {
      tan_theta = 0.0;
      axis_extents[0] = extent.y;
      axis_extents[1] = extent.x;
    }
  else
    { 
      double Wo = extent.x, Ho = extent.y;
      double R = Wo / Ho;
      double R_sq = R*R;
      double Q = ((R_sq*R_sq + 2.0*gamma_sq*R_sq + 1.0) /
                  (2.0*R_sq*(1.0-gamma_sq)));
      double rho_sq = Q + sqrt(Q*Q-1.0);
      tan_theta = (R_sq - rho_sq) / ((1.0 + rho_sq) * R * gamma);
      if ((tan_theta < -1.0) || (tan_theta > 1.0))
        {
          rho_sq = 1.0 / rho_sq;
          tan_theta = (R_sq - rho_sq) / ((1.0 + rho_sq) * R * gamma);
        }
      double rho = sqrt(rho_sq);
      double tan_phi = rho * tan_theta;
      double tan_zeta = tan_theta / rho;
      axis_extents[0] = Ho * sqrt((1.0 + tan_theta*tan_theta) /
                                  (1.0 + tan_phi*tan_phi));
      axis_extents[1] = Wo * sqrt((1.0 + tan_theta*tan_theta) /
                                  (1.0 + tan_zeta*tan_zeta));
    }
  return true;
}

/*****************************************************************************/
/*                        jpx_roi::fix_inconsistencies                       */
/*****************************************************************************/

void jpx_roi::fix_inconsistencies()
{
  if (region.size.x < 1) region.size.x = 1;
  if (region.size.y < 1) region.size.y = 1;
  if (is_elliptical)
    {
      region.size.x |= 1;  region.size.y |= 1;
      if (elliptical_skew.x || elliptical_skew.y)
        {
          double gamma;  kdu_coords extent;
          if ((!compute_gamma_and_extent(gamma,extent)) ||
              (elliptical_skew.x <= -extent.x) ||
              (elliptical_skew.x >= extent.x) ||
              (elliptical_skew.y <= -extent.y) ||
              (elliptical_skew.y >= extent.y))
            {
              elliptical_skew.x = (int) floor(0.5+gamma*extent.x);
              elliptical_skew.y = (int) floor(0.5+gamma*extent.y);
            }
        }
    }
  else if (flags & JPX_QUADRILATERAL_ROI)
    init_quadrilateral(vertices[0],vertices[1],vertices[2],vertices[3],
                       is_encoded,coding_priority);
}

/*****************************************************************************/
/*                            jpx_roi::clip_region                           */
/*****************************************************************************/

void jpx_roi::clip_region()
{
  kdu_coords min=region.pos, max=min+region.size-kdu_coords(1,1);
  if (min.x < 0) min.x = 0;
  if (min.y < 0) min.y = 0;
  if (max.x < min.x) max.x = min.x;
  if (max.y < min.y) max.y = min.y;
  if (max.x > 0x7FFFFFFE) max.x = 0x7FFFFFFE;
  if (max.y > 0x7FFFFFFE) max.y = 0x7FFFFFFE;
  if (min.x > max.x) min.x = max.x;
  if (min.y > max.y) min.y = max.y;
  kdu_dims new_region;
  new_region.pos = min;
  new_region.size = max-min+kdu_coords(1,1);
  if (is_elliptical)
    {
      kdu_coords new_centre, new_extent;
      new_extent.x = new_region.size.x >> 1;
      new_extent.y = new_region.size.y >> 1;
      new_centre = new_region.pos + new_extent;
      if (new_extent.x < 1) new_extent.x = 1;
      if (new_extent.y < 1) new_extent.y = 1;
      min = new_centre - new_extent;
      max = new_centre + new_extent;
      if (max.x > 0x7FFFFFFE)
        new_centre.x = 0x7FFFFFFE - new_extent.x;
      else if (min.x < 0)
        new_centre.x = new_extent.x;
      if (max.y > 0x7FFFFFFE)
        new_centre.y = 0x7FFFFFFE - new_extent.y;
      else if (min.y < 0)
        new_centre.y = new_extent.y;
      new_region.pos = new_centre - new_extent;
      new_region.size = new_extent + new_extent + kdu_coords(1,1);
      if (new_region == region)
        return;
      if (elliptical_skew.x || elliptical_skew.y)
        {
          // Recompute the skew parameters
          kdu_coords extent, skew=elliptical_skew;
          double gamma;
          compute_gamma_and_extent(gamma,extent);
          double x = gamma*new_extent.x, y=gamma*new_extent.y;
          skew.x = (int) floor(x+0.5);
          skew.y = (int) floor(y+0.5);
          if (skew.x <= -new_extent.x) skew.x = 1-new_extent.x;
          if (skew.x >= new_extent.x) skew.x = new_extent.x-1;
          if (skew.y <= -new_extent.y) skew.y = 1-new_extent.y;
          if (skew.y >= new_extent.y) skew.y = new_extent.y-1;
          elliptical_skew = skew;
        }
      region = new_region;
    }
  else if (!(flags & JPX_QUADRILATERAL_ROI))
    { // Simple rectangular region
      region = new_region;
    }
  else
    { 
      int p;
      for (p=0; p < 4; p++)
        { 
          if (vertices[p].x < 0) vertices[p].x = 0;
          if (vertices[p].x > 0x7FFFFFFE) vertices[p].x = 0x7FFFFFFE;
          if (vertices[p].y < 0) vertices[p].y = 0;
          if (vertices[p].y > 0x7FFFFFFE) vertices[p].y = 0x7FFFFFFE;
          if (p == 0)
            min = max = vertices[p];
          else
            { 
              if (vertices[p].x < min.x) min.x = vertices[p].x;
              if (vertices[p].x > max.x) max.x = vertices[p].x;
              if (vertices[p].y < min.y) min.y = vertices[p].y;
              if (vertices[p].y > max.y) max.y = vertices[p].y;
            }
        }
      region.pos = min;
      region.size = max-min+kdu_coords(1,1);
    }
}


/*****************************************************************************/
/*                          jpx_roi::check_geometry                          */
/*****************************************************************************/

bool
  jpx_roi::check_geometry() const
{
  // Start by checking that the bounding box is legal
  if ((region.pos.x < 0) || (region.pos.y < 0) ||
      (region.size.x <= 0) || (region.size.y <= 0) ||
      ((region.pos.x+region.size.x) < 0) ||
      ((region.pos.y+region.size.y) < 0))
    return false;
  
  if (is_elliptical)
    { // Check the skew parameters
      kdu_coords extent;
      extent.x = region.size.x >> 1;  extent.y = region.size.y >> 1;
      if ((extent.x < 1) || (extent.y < 1) ||
          (region.size.x != (2*extent.x+1)) ||
          (region.size.y != (2*extent.y+1)) ||
          (elliptical_skew.x <= -extent.x) ||
          (elliptical_skew.x >= extent.x) ||
          (elliptical_skew.y <= -extent.y) ||
          (elliptical_skew.y >= extent.y))
        return false;
      if (elliptical_skew.x || elliptical_skew.y)
        { 
          double gamma;
          if (!compute_gamma_and_extent(gamma,extent))
            return false;
        }
      return true;
    }
  if (!(flags & JPX_QUADRILATERAL_ROI))
    return true;
  
  int p;
  for (p=0; p < 4; p++)
    if ((vertices[p].x < 0) || (vertices[p].y < 0) ||
        (vertices[p].x > 0x7FFFFFFE) || (vertices[p].y > 0x7FFFFFFE))
      return false;
  
  /* To determine whether the vertices are ordered in a clockwise direction,
     we measure the orthogonal (righward) distance of the points
     A = vertices[1] and B = vertices[3] from the directed line segment
     (U -> W) where U = vertices[0] and W = vertices[2].  The orientation is
     clockwise if and only if B lies at least as far to the right as A.
        We use the following mathematical result to assess rightward orthogonal
     distance.  If U, W and Z are three vectors, a perpendicular vector in
     the rightward direction from line segment (U -> W) is V, where
     Vy = Wx-Ux and Vx = -(Wy-Uy).  The rightward displacement of Z relative
     to the directed line segment (U -> W) is then given by
       d = [(Wx-Ux)(Zy-Uy) - (Wy-Uy)(Zx-Ux)] / |V|
   */
  kdu_long dx = vertices[2].x; dx -= vertices[0].x; // this is Wx-Ux
  kdu_long dy = vertices[2].y; dy -= vertices[0].y; // this is Wy-Uy
  kdu_coords a_vec = vertices[1] - vertices[0]; // (Z-U) for point A
  kdu_coords b_vec = vertices[3] - vertices[0]; // (Z-U) for point B
  if ((dx*(b_vec.y-a_vec.y) - dy*(b_vec.x-a_vec.x)) < 0)
    return false;

  if (check_edge_intersection(0,vertices[2],vertices[3]))
    return false; // Checks intersection between edges 0-1 and 2-3
  if (check_edge_intersection(3,vertices[1],vertices[2]))
    return false; // Checks intersection between edges 3-0 and 1-2
  
  return true;
}

/*****************************************************************************/
/*                     jpx_roi::check_edge_intersection                      */
/*****************************************************************************/

bool
  jpx_roi::check_edge_intersection(int n, kdu_coords C, kdu_coords D) const
{
  kdu_coords A = vertices[n], B = vertices[(n+1) & 3];
  kdu_long AmB_x=A.x-B.x, AmB_y=A.y-B.y;
  kdu_long DmC_x=D.x-C.x, DmC_y=D.y-C.y;
  kdu_long AmC_x=A.x-C.x, AmC_y=A.y-C.y;
  
  kdu_long det = AmB_x * DmC_y - DmC_x * AmB_y;
  kdu_long t_val = DmC_y*AmC_x - DmC_x * AmC_y;
  kdu_long u_val = AmB_x*AmC_y - AmB_y * AmC_x;
  if (det < 0.0)
    { det = -det; t_val = -t_val; u_val = -u_val; }
  return ((t_val > 0) && (t_val < det) && (u_val > 0) && (u_val < det));
}

/*****************************************************************************/
/*                          jpx_roi::measure_span                            */
/*****************************************************************************/

int
  jpx_roi::measure_span(double &tightest_width, double &tightest_length) const
{
  if (is_elliptical)
    {
      kdu_coords centre;
      double axis_extents[2], tan_theta;
      get_ellipse(centre,axis_extents,tan_theta);
      if (axis_extents[0] < axis_extents[1])
        {
          tightest_width = 1+2*axis_extents[0];
          tightest_length = 1+2*axis_extents[1];
          return 1;
        }
      else
        {
          tightest_width = 1+2*axis_extents[1];
          tightest_length = 1+2*axis_extents[0];
          return 0;          
        }
    }
  if (!(flags & JPX_QUADRILATERAL_ROI))
    {
      if (region.size.x < region.size.y)
        { // Longer dimension is aligned vertically
          tightest_width = region.size.x; tightest_length = region.size.y;
          return 1;
        }
      else
        { // Longer dimension is aligned horizontally
          tightest_width = region.size.y; tightest_length = region.size.x;
          return 0;
        }
    }

  int n, tightest_n=0;
  tightest_width = tightest_length = 0.0;
  
  for (n=0; n < 4; n++)
    { // Consider the edge U->W running from vertex[n] to vertex[(n+1) mod 4]
      
      /* We use the following mathematical result to assess rightward
         orthogonal distance.  A perpendicular vector in the rightward
         direction from line segment (U -> W) is V, where Vy = Wx-Ux and
         Vx = -(Wy-Uy).  The rightward displacement of a third point Z,
         relative to the directed line segment (U -> W) is then given by
         d = [(Wx-Ux)(Zy-Uy) - (Wy-Uy)(Zx-Ux)] / |V|. */
      
      int m = (n+1) & 3;
      if (vertices[m] == vertices[n])
        continue;
      double Ux=vertices[n].x,     Uy=vertices[n].y;
      double WUx=vertices[m].x-Ux, WUy=vertices[m].y-Uy;
      double inv_V_mag = 1.0 / sqrt(WUx*WUx + WUy*WUy);
           // This is both the reciprocal of the length of vector V and of
           // of vector U->W.
      
      double ZUx, ZUy, d1, d2;
      m = (m+1) & 3; // Consider the next vertex as Z
      ZUx=vertices[m].x-Ux;  ZUy=vertices[m].y-Uy;
      d1 = (WUx*ZUy - WUy*ZUx) * inv_V_mag;
      m = (m+1) & 3; // Consider the final vertex as Z
      ZUx=vertices[m].x-Ux;  ZUy=vertices[m].y-Uy;
      d2 = (WUx*ZUy - WUy*ZUx) * inv_V_mag;
      
      double width;
      if (d1 <= d2)
        {
          if (d1 < 0.0)
            width = (d2 > 0.0)?(d2-d1):-d1;
          else
            width = d2;
        }
      else
        { // d2 < d1
          if (d2 < 0.0)
            width = (d1 > 0.0)?(d1-d2):-d2;
          else
            width = d1;
        }
      
      if ((n > 0) && (width >= tightest_width))
        continue; // Not a candidate
      
      tightest_width = width;
      tightest_n = n;

      // Calculate length of this bounding rectangle by projecting each
      // vertex onto the vector U->W.  The unit vector in this direction has
      // coordinates (WUx*inv_V_mag, WUy*inv_V_mag).
      double proj, proj_min=0.0, proj_max=0.0;
      for (int p=0; p < 4; p++)
        {
          proj = WUx*vertices[p].x + WUy*vertices[p].y;
          if (p == 0)
            proj_min = proj_max = proj;
          else if (proj < proj_min)
            proj_min = proj;
          else if (proj > proj_max)
            proj_max = proj;
        }
      tightest_length = (proj_max-proj_min) * inv_V_mag;
    }
  tightest_width += 1.0;  tightest_length += 1.0; // Account for pixel size
  return tightest_n;
}

/*****************************************************************************/
/*                          jpx_roi::measure_area                            */
/*****************************************************************************/

double
  jpx_roi::measure_area(double &centroid_x, double &centroid_y) const
{
  centroid_x = region.pos.x + 0.5*(region.size.x-1);
  centroid_y = region.pos.y + 0.5*(region.size.y-1);
  double area = (double) region.area();  
  if (is_elliptical)
    {
      kdu_coords extent; double gamma;
      compute_gamma_and_extent(gamma,extent);
      area = JX_PI * (extent.x+0.5) * (extent.y+0.5) * sqrt(1-gamma*gamma);
      return area;
    }
  if (!(flags & JPX_QUADRILATERAL_ROI))
    return area;

  // If we get here, we have a general quadrilateral.  We analyze this by
  // dividing it into 3 trapezoids, each of which have their top and bottom
  // edges aligned horizontally.
  jx_trapezoid trap[3];
  trap[0].init(vertices[0],vertices[3],vertices[0],vertices[1]);
  if ((vertices[2].y < vertices[1].y) && (vertices[2].y < vertices[3].y))
    { // Vertex 2 lies above vertices 1 and 3 which join to the top
      kdu_long V1m0_y=((kdu_long) vertices[1].y) - ((kdu_long) vertices[0].y);
      kdu_long V1m0_x=((kdu_long) vertices[1].x) - ((kdu_long) vertices[0].x);
      kdu_long V2m0_y=((kdu_long) vertices[2].y) - ((kdu_long) vertices[0].y);
      kdu_long V2m0_x=((kdu_long) vertices[2].x) - ((kdu_long) vertices[0].x);
      kdu_long V3m0_y=((kdu_long) vertices[3].y) - ((kdu_long) vertices[0].y);
      kdu_long V3m0_x=((kdu_long) vertices[3].x) - ((kdu_long) vertices[0].x);
      if (((V2m0_y*V3m0_x) <= (V2m0_x*V3m0_y)) &&
          ((V2m0_y*V1m0_x) >= (V2m0_x*V1m0_y))) 
        { // Vertex 2 lies between the left & right edges of trap-0
          trap[0].max_y = vertices[2].y;
          trap[1].init(vertices[0],vertices[3],vertices[2],vertices[3]);
          trap[2].init(vertices[2],vertices[1],vertices[0],vertices[1]);
        }
      else if (vertices[3].y < vertices[1].y)
        { // Vertex 2 lies to the left of the left edge of trap-0
          trap[1].init(vertices[2],vertices[1],vertices[2],vertices[3]);
          trap[2].init(vertices[2],vertices[1],vertices[0],vertices[1]);
          trap[2].min_y = vertices[3].y;
        }
      else
        { // Vertex 2 lies to the right of the right edge of trap-0
          trap[1].init(vertices[2],vertices[1],vertices[2],vertices[3]);
          trap[2].init(vertices[0],vertices[3],vertices[2],vertices[3]);
          trap[2].min_y = vertices[1].y;
        }
    }
  else if (vertices[3].y < vertices[1].y)
    { // Vertex 3 lies above vertices 1 and 3
      trap[1].init(vertices[3],vertices[2],vertices[0],vertices[1]);
      if (vertices[2].y < vertices[1].y)
        trap[2].init(vertices[2],vertices[1],vertices[0],vertices[1]);
      else
        trap[2].init(vertices[3],vertices[2],vertices[1],vertices[2]);
    }
  else
    { // Vertex 1 lies above vertices 2 and 3
      trap[1].init(vertices[0],vertices[3],vertices[1],vertices[2]);
      if (vertices[2].y < vertices[3].y)
        trap[2].init(vertices[0],vertices[3],vertices[2],vertices[3]);
      else
        trap[2].init(vertices[3],vertices[2],vertices[1],vertices[2]);    
    }
  
  double cumulative_area=0.0;
  double area_weighted_cx=0.0, area_weighted_cy=0.0;
  for (int t=0; t < 3; t++)
    {
      if (trap[t].min_y > trap[t].max_y)
        continue;
      int min_y = trap[t].min_y, max_y = trap[t].max_y;
      kdu_coords delta_L=trap[t].left2 - trap[t].left1;
      kdu_coords delta_R=trap[t].right2 - trap[t].right1;
      double grad_L = delta_L.x / (double) delta_L.y;
      double grad_R = delta_R.x / (double) delta_R.y;
      double x1_L = trap[t].left1.x + grad_L*(min_y-trap[t].left1.y);
      double x1_R = trap[t].right1.x + grad_R*(min_y-trap[t].right1.y);
      double height = max_y-min_y;
      double x2_L = x1_L + grad_L * height;
      double x2_R = x1_R + grad_R * height;
      double width1 = x1_R-x1_L, width2 = x2_R-x2_L;
      double local_area = (height+1.0) * (1.0 + 0.5 * (width1+width2));
      cumulative_area += local_area;
      double delta_width = width2-width1;
      double delta_cy_over_height =
        (0.5*width1 + (1/3.0)*delta_width) / (width1 + 0.5*delta_width);
      double x1 = 0.5*(x1_L+x1_R), x2 = 0.5*(x2_L+x2_R);
      double cx = x1 + (x2-x1) * delta_cy_over_height;
      double cy = min_y + height * delta_cy_over_height;
      area_weighted_cx += cx*local_area;
      area_weighted_cy += cy*local_area;
    }
  
  if (cumulative_area > 0.0)
    { // Otherwise, the quadrilateral has height 0 and the parameters
      // previously computed from the bounding box are correct.
      area = cumulative_area;
      centroid_x = area_weighted_cx * (1.0 / cumulative_area);
      centroid_y = area_weighted_cy * (1.0 / cumulative_area);
    }
  return area;
}

/*****************************************************************************/
/*                            jpx_roi::contains                              */
/*****************************************************************************/

bool
  jpx_roi::contains(kdu_coords point) const
{
  kdu_coords off = point - region.pos;
  if ((off.x < 0) || (off.x >= region.size.x) ||
      (off.y < 0) || (off.y >= region.size.y))
    return false;
  if (is_elliptical)
    {
      kdu_coords extent;
      double gamma;
      compute_gamma_and_extent(gamma,extent);
      double Ho=extent.y, Wo=extent.x;
      double Wi = Wo*sqrt(1.0-gamma*gamma);
      double alpha = gamma * Wo / Ho;
      double y = off.y - Ho;
      double x = off.x - Wo + alpha*y;
        // Now we just need to check whether
        //       x^2/Wi^2 + y^2/Ho^2 <= 1
        // Equivalently, we check whether
        //       x^2 * Ho^2 + y^2 * Wi^2 <= Wi^2 * Ho^2
        // To make the test a little more resilient to rounding errors, we
        // will allow for the point to be up to half a sample away from the
        // boundary, this means that we first need to bring x and y in towards
        // (0,0) by 0.5.
    
      if (x > 0.5)
        x -= 0.5;
      else if (x < -0.5)
        x += 0.5;
      if (y > 0.5)
        y -= 0.5;
      else if (y < -0.5)
        y += 0.5;
      double Wi_sq=Wi*Wi, Ho_sq = Ho*Ho;
      if ((x*x*Ho_sq + y*y*Wi_sq) > Ho_sq * Wi_sq)
        return false;
    }
  if (!(flags & JPX_QUADRILATERAL_ROI))
    return true;
  
  // If we get here, we need to determine membership of a general
  // quadrilateral defined by the `vertices' member.  We will do this by
  // dividing the quadrilateral into 3 trapezoids, each of which have
  // their top and bottom edges aligned horizontally.
  jx_trapezoid trap[3];
  trap[0].init(vertices[0],vertices[3],vertices[0],vertices[1]);
  if ((vertices[2].y < vertices[1].y) && (vertices[2].y < vertices[3].y))
    { // Vertex 2 lies above vertices 1 and 3 which join to the top
      kdu_long V1m0_y=((kdu_long) vertices[1].y) - ((kdu_long) vertices[0].y);
      kdu_long V1m0_x=((kdu_long) vertices[1].x) - ((kdu_long) vertices[0].x);
      kdu_long V2m0_y=((kdu_long) vertices[2].y) - ((kdu_long) vertices[0].y);
      kdu_long V2m0_x=((kdu_long) vertices[2].x) - ((kdu_long) vertices[0].x);
      kdu_long V3m0_y=((kdu_long) vertices[3].y) - ((kdu_long) vertices[0].y);
      kdu_long V3m0_x=((kdu_long) vertices[3].x) - ((kdu_long) vertices[0].x);
      if (((V2m0_y*V3m0_x) <= (V2m0_x*V3m0_y)) &&
          ((V2m0_y*V1m0_x) >= (V2m0_x*V1m0_y))) 
        { // Vertex 2 lies between the left & right edges of trap-0
          trap[0].max_y = vertices[2].y;
          trap[1].init(vertices[0],vertices[3],vertices[2],vertices[3]);
          trap[2].init(vertices[2],vertices[1],vertices[0],vertices[1]);
        }
      else if (vertices[3].y < vertices[1].y)
        { // Vertex 2 lies to the left of the left edge of trap-0
          trap[1].init(vertices[2],vertices[1],vertices[2],vertices[3]);
          trap[2].init(vertices[2],vertices[1],vertices[0],vertices[1]);
          trap[2].min_y = vertices[3].y;
        }
      else
        { // Vertex 2 lies to the right of the right edge of trap-0
          trap[1].init(vertices[2],vertices[1],vertices[2],vertices[3]);
          trap[2].init(vertices[0],vertices[3],vertices[2],vertices[3]);
          trap[2].min_y = vertices[1].y;
        }
    }
  else if (vertices[3].y < vertices[1].y)
    { // Vertex 3 lies above vertices 1 and 3
      trap[1].init(vertices[3],vertices[2],vertices[0],vertices[1]);
      if (vertices[2].y < vertices[1].y)
        trap[2].init(vertices[2],vertices[1],vertices[0],vertices[1]);
      else
        trap[2].init(vertices[3],vertices[2],vertices[1],vertices[2]);
    }
  else
    { // Vertex 1 lies above vertices 2 and 3
      trap[1].init(vertices[0],vertices[3],vertices[1],vertices[2]);
      if (vertices[2].y < vertices[3].y)
        trap[2].init(vertices[0],vertices[3],vertices[2],vertices[3]);
      else
        trap[2].init(vertices[3],vertices[2],vertices[1],vertices[2]);    
    }
  for (int t=0; t < 3; t++)
    {
      if ((point.y < trap[t].min_y) || (point.y > trap[t].max_y))
        continue;
      double x1, x2, y1, y2, x, y=point.y;
          
      x = point.x + 0.5; // Right edge of pixel must be insider border
      x1 = trap[t].left1.x;  x2 = trap[t].left2.x;
      if (trap[t].left1.y < trap[t].left2.y)
        {
          x-= x1;  y1 = trap[t].left1.y;  y2 = trap[t].left2.y;
          if (((x * (y2-y1)) < ((y-0.5-y1)*(x2-x1))) &&
              ((x * (y2-y1)) < ((y+0.5-y1)*(x2-x1))))
            continue; // Right edge of point lies entirely to left of border
        }
      else if ((x < x1) && (x < x2))
        continue; // Point lies to left of horiz. left edge segment
          
      x = point.x - 0.5; // Left edge of pixel must be insider border
      x1 = trap[t].right1.x;  x2 = trap[t].right2.x;
      if (trap[t].right1.y < trap[t].right2.y)
        {
          x -= x1;  y1 = trap[t].right1.y;  y2 = trap[t].right2.y;
          if (((x * (y2-y1)) > ((y-0.5-y1)*(x2-x1))) &&
              ((x * (y2-y1)) > ((y+0.5-y1)*(x2-x1))))
            continue; // Left edge of point lies entirely to right of border
        }
      else if ((x > x1) && (x > x2))
        continue; // Point lies to right of horiz. right edge segment
    
      return true; // If we get here, the point belongs to trapezoid trap[t].
    }
  return false;
}

/*****************************************************************************/
/*                    jpx_roi::find_boundary_projection                      */
/*****************************************************************************/

int
  jpx_roi::find_boundary_projection(double x0, double y0,
                                    double &xp, double &yp,
                                    double max_distance,
                                    double tolerance) const
{
  double max_dist_sq = max_distance*max_distance;
  if ((!is_elliptical) && !(flags & JPX_QUADRILATERAL_ROI))
    { // Start with rectangular regions
      double mid_x = region.pos.x + 0.5*(region.size.x-1);
      double mid_y = region.pos.y + 0.5*(region.size.y-1);
      int result;
      if (x0 < mid_x)
        { xp = region.pos.x; result = 0; }
      else
        { xp = region.pos.x+region.size.x-1; result = 1; }
      if (y0 < mid_y)
        yp = region.pos.y;
      else
        { yp = region.pos.y+region.size.y-1; result = 3-result; }
      double dist_sq = (xp-x0)*(xp-x0) + (yp-y0)*(yp-y0);
      return (dist_sq > max_dist_sq)?-1:result;
    }
  if (flags & JPX_QUADRILATERAL_ROI)
    { // General quadrilaterals are not too bad; project onto each edge in turn
      // For an edge (A,B), we first find t such that the vector from (x0,y0)
      // to A+t(B-A) is orthogonal to (A,B); after that the projection onto
      // line segment (A,B) is: A+t(B-A) if 0 < t < 1; A if t<=0; B if t>=1.
      // The equation we need to solve is:
      //    (x0-Ax-t(Bx-Ax))*(Bx-Ax) + (y0-Ay-t(By-Ay))*(By-Ay) = 0.
      // That is,
      //    (x0-Ax)(Bx-Ax) + (y0-Ay)(By-Ay) = t*[(Bx-Ax)^2 + (By-Ay)^2].
      int p, best_p=0;
      double best_dist_sq=0.0;
      for (p=0; p < 4; p++)
        { 
          double Ax=vertices[p].x, Ay=vertices[p].y;
          double Bx=vertices[(p+1)&3].x, By=vertices[(p+1)&3].y;
          double den = (Bx-Ax)*(Bx-Ax) + (By-Ay)*(By-Ay);
          double num = (x0-Ax)*(Bx-Ax) + (y0-Ay)*(By-Ay);
          double t;
          if (num <= 0.0)
            t = 0.0; // Solution would involve t <= 0
          else if (num >= den)
            t = 1.0; // Solution would involve t >= 1
          else
            t = num/den; // Division is safe since above tests failed
          double x = Ax+t*(Bx-Ax);
          double y = Ay+t*(By-Ay);
          double dist_sq = (x0-x)*(x0-x) + (y0-y)*(y0-y);
          if ((p==0) || (dist_sq < best_dist_sq))
            { xp = x; yp = y; best_dist_sq = dist_sq; best_p = p; }
        }
      return (best_dist_sq > max_dist_sq)?-1:best_p;
    }
  
  /* If we get here, we are dealing with an ellipse; these are hard.
     Here is the theory:
        First, let `a' be the horizontal skewing parameter and Ho and
     Wi be half the exterior height and half the interior width,
     respectively.  The interior of the ellipse is described by
        (x+ay)^2 / Wi^2 + y^2 / Ho^2 <= 1
     where (x,y) is expressed relative to the centre of the ellipse.
     Equivalently,
        A(x+ay)^2 + By^2 <= C, with A=Ho^2, B=Wi^2 and C=Ho^2*Wi^2.
     So, assuming we have already subtracted the ellipse's centre coordinates
     from x0 and y0, we want to find (x,y) which minimizes
        (x-x0)^2+(y-y0)^2, subject to A(x+ay)^2 + By^2 = C.
        Using the method of Lagrange multipliers, our problem is equivalent to
     minimizing
        (x-x0)^2+(y-y0)^2 + lambda * [ A(x+ay)^2 + By^2 - C]
     for some lambda, where lambda is positive if (x0,y0) lies outside the
     ellipse and negative if (x0,y0) lies inside the ellipse.  The solution
     to this quadratic problem is readily obtained by differentiation.
     Specifically, we must have
        (x-x0) + lambda * A(x+ay) = 0
        (y-y0) + lambda * Aa(x+ay) + lambda * B * y = 0
     So
        | 1+lambda*A          lambda*A*a        |   | x |   | x0 |
        |                                       | * |   | = |
        | lambda*A*a   1+lambda*a^2*A+lambda*B  |   | y |   | y0 |
     which is readily solved to yield
                  | 1+lambda*(a^2*A+B)   -lambda*A*a |
        (x,y)^t = |    -lambda*A*a        1+lambda*A | * (x0,y0)^2
                  ------------------------------------
             (1+lambda*A)(1+lambda*(a^2*A+B))-(lambda*A*a)^2
     Now all we need to do is search for a value of lambda which yields
     a solution to A(x+ay)^2 + By^2 <= C.  The implementation here, uses
     a bisection search, starting with lambda=0 (to determine whether or
     not (x0,y0) lies inside the ellipse).  This approach is fine since
     we do not need very high accuracy.  Once the value of
     |A(x+ay)^2 + By^2 - C| is smaller than (2A|x+ay|+2B|y|)*`tolerance',
     we can stop.
  */
     
  kdu_coords extent, centre;
  centre.x = region.pos.x + (region.size.x>>1);
  centre.y = region.pos.y + (region.size.y>>1);
  double gamma;
  compute_gamma_and_extent(gamma,extent);
  double ho = extent.y;
  double ho_sq = ho*ho;
  double wo = extent.x;
  double wi_sq = wo*wo*(1.0-gamma*gamma);
  double alpha = elliptical_skew.x / ho;
  x0 -= centre.x;  y0 -= centre.y;
  double A = ho_sq, B = wi_sq, C = ho_sq*wi_sq;
  
  double min_lambda=0.0, max_lambda=0.0;
  double Cval = A*(x0+alpha*y0)*(x0+alpha*y0) + B*y0*y0;
  bool have_both_bounds = false;
  bool inside_ellipse = false;
  if (Cval < C) // (x0,y0) is inside the ellipse
    { min_lambda = -1.0/(A+B); inside_ellipse = true; }
  else // (x0,y0) is outside the ellipes
    { max_lambda = 1.0/(A+B); inside_ellipse = false; }
  double Ctol = 0.0; // Recalculate this inside the loop below
  if (tolerance <= 0.0)
    tolerance = 0.01; // Just in case
  double dist_sq = 0.0; // Computed as we go
  do {
    double lambda = (min_lambda+max_lambda)*0.5;
    double A11=1+lambda*A, A12=lambda*A*alpha, A22=1+lambda*(alpha*alpha*A+B);
    double det = A11*A22 - A12*A12;
    if (det == 0.0)
      return -1; // This can only potentially happen if (x0,y0) is inside
    double inv_det = 1.0/det;
    xp = (A22*x0-A12*y0)*inv_det;
    yp = (A11*y0-A12*x0)*inv_det;
    dist_sq = (xp-x0)*(xp-x0) + (yp-y0)*(yp-y0);
    double x_term = xp+alpha*yp;
    double x_sensitivity = A*x_term, y_sensitivity = B*yp;
    Cval = x_term*x_sensitivity + yp*y_sensitivity;
    Ctol = (x_sensitivity<0.0)?(-x_sensitivity):x_sensitivity;
    Ctol += (y_sensitivity<0.0)?(-y_sensitivity):y_sensitivity;
    if (Ctol < 0.1)
      Ctol = 0.1; // Just in case ellipse has zero size or something like that
    Ctol *= tolerance;
    if (!have_both_bounds)
      {
        if (inside_ellipse)
          {
            if (Cval < C)
              min_lambda *= 2.0; // Still inside the ellipse
            else
              { have_both_bounds=true; min_lambda=lambda; }
          }
        else
          { 
            if (Cval > C)
              max_lambda *= 2.0; // Still outside the ellipse
            else
              { have_both_bounds=true; max_lambda=lambda; }
          }
      }
    else
      {
        if (Cval < C)
          max_lambda = lambda;
        else
          min_lambda = lambda;
        if (inside_ellipse && (Cval < C) && (dist_sq > max_dist_sq))
          return -1; // Projection onto a smaller ellipse already too far away
        else if ((!inside_ellipse) && (Cval > C) && (dist_sq > max_dist_sq))
          return -1; // Projection onto a larger ellipse already too far away
      }
  } while ((Cval < (C-Ctol)) || (Cval > (C+Ctol)));
  xp += centre.x;
  yp += centre.y;
  
  return (dist_sq > max_dist_sq)?-1:0;
}


/* ========================================================================= */
/*                              jpx_roi_editor                               */
/* ========================================================================= */

/*****************************************************************************/
/*                    jpx_roi_editor::set_max_undo_history                   */
/*****************************************************************************/

void jpx_roi_editor::set_max_undo_history(int history)
{
  max_undo_elements = history;
  if (num_undo_elements > history)
    {
      num_undo_elements = history;
      jpx_roi_editor *scan, *last=this;
      for (; history > 0; history--, last=last->prev);
      while ((scan=last->prev) != NULL)
        {
          last->prev = scan->prev;
          scan->is_current = false;
          delete scan;
        }
    }
}

/*****************************************************************************/
/*                           jpx_roi_editor::undo                            */
/*****************************************************************************/

kdu_dims jpx_roi_editor::undo()
{
  kdu_dims result = cancel_selection();
  jpx_roi_editor *undo_elt = prev;
  if (undo_elt == NULL)
    return result;

  kdu_dims bb;
  this->get_bounding_box(bb,true);
  result.augment(bb);
  
  // Make a copy of `this'
  jpx_roi_editor save_this = *this;
  save_this.is_current = false; // So destructor will do no harm
  
  // Make `undo_elt' current
  *this = *undo_elt;
  this->is_current = true;
  this->max_undo_elements = save_this.max_undo_elements;
  this->num_undo_elements = save_this.num_undo_elements-1;
  this->mode = save_this.mode;
  if (this->prev != NULL)
    this->prev->next = this;
  
  // Move `save_this' into the most immediate redo position, re-using the
  // `undo_elt' record for this purpose.
  jpx_roi_editor *redo_elt = undo_elt;
  *redo_elt = save_this;
  this->next = redo_elt;
  redo_elt->prev = this;
  if (redo_elt->next != NULL)
    redo_elt->next->prev = redo_elt;
  
  this->get_bounding_box(bb,true);
  result.augment(bb);
  path_edge_flags_valid = shared_edge_flags_valid = false;
  return result;  
}

/*****************************************************************************/
/*                           jpx_roi_editor::redo                            */
/*****************************************************************************/

kdu_dims jpx_roi_editor::redo()
{
  kdu_dims result = cancel_selection();
  jpx_roi_editor *redo_elt = next;
  if (redo_elt == NULL)
    return result;
  
  kdu_dims bb;
  this->get_bounding_box(bb,true);
  result.augment(bb);
  
  // Make a copy of `this'
  jpx_roi_editor save_this = *this;
  save_this.is_current = false; // So destructor will do no harm
  
  // Make `redo_elt' current
  *this = *redo_elt;
  this->is_current = true;
  this->max_undo_elements = save_this.max_undo_elements;
  this->num_undo_elements = save_this.num_undo_elements+1;
  this->mode = save_this.mode;
  if (this->next != NULL)
    this->next->prev = this;
  
  // Move `save_this' into the most immediate undo position, re-using the
  // `redo_elt' record for this purpose.
  jpx_roi_editor *undo_elt = redo_elt;
  *undo_elt = save_this;
  this->prev = undo_elt;
  undo_elt->next = this;
  if (undo_elt->prev != NULL)
    undo_elt->prev->next = undo_elt;
  
  this->get_bounding_box(bb,true);
  result.augment(bb);
  path_edge_flags_valid = shared_edge_flags_valid = false;
  return result;
}

/*****************************************************************************/
/*                    jpx_roi_editor::push_current_state                     */
/*****************************************************************************/

void jpx_roi_editor::push_current_state()
{
  // Remove the redo list
  jpx_roi_editor *scan;
  while ((scan=next) != NULL)
    { next = scan->next; scan->is_current=false; delete scan; }
  
  if (max_undo_elements <= 0)
    return;
  
  // See if we need to drop the least recent element from the undo list
  if (num_undo_elements >= max_undo_elements)
    {
      num_undo_elements = max_undo_elements-1;
      int count = num_undo_elements;
      jpx_roi_editor *last=this;
      for (; count > 0; count--, last=last->prev);
      while ((scan=last->prev) != NULL)
        {
          last->prev = scan->prev;
          scan->is_current = false;
          delete scan;
        }
    }
  
  // Make a new element for the undo list
  jpx_roi_editor *undo_elt = new jpx_roi_editor;
  *undo_elt = *this;
  undo_elt->is_current = false;
  if (undo_elt->prev != NULL)
    undo_elt->prev->next = undo_elt;
  undo_elt->next = this;
  prev = undo_elt;
  this->num_undo_elements++;
  
  // Remove selection properties from `undo_elt'
  undo_elt->anchor_idx = undo_elt->region_idx = undo_elt->edge_idx = -1;
  memset(undo_elt->drag_flags,0,(size_t)undo_elt->num_regions);
  undo_elt->path_edge_flags_valid = undo_elt->shared_edge_flags_valid = false;
}

/*****************************************************************************/
/*                          jpx_roi_editor::equals                           */
/*****************************************************************************/

bool jpx_roi_editor::equals(const jpx_roi_editor &rhs) const
{
  if (num_regions != rhs.num_regions)
    return false;
  for (int n=0; n < num_regions; n++)
    if (regions[n] != rhs.regions[n])
      return false;
  return true;
}

/*****************************************************************************/
/*                           jpx_roi_editor::init                            */
/*****************************************************************************/

void jpx_roi_editor::init(const jpx_roi *regs, int num_regions)
{
  if ((num_regions < 0) || (num_regions > 255))
    { KDU_ERROR_DEV(e,0x18021001); e <<
      KDU_TXT("Invalid set of ROI regions supplied to "
              "`jpx_roi_editor::init'.");
    }
  this->num_regions = num_regions;
  for (int n=0; n < num_regions; n++)
    { 
      jpx_roi *dest = this->regions+n;
      *dest = regs[n];
      update_extremities(dest);
    }
  region_idx = edge_idx = anchor_idx = -1;
  path_edge_flags_valid = shared_edge_flags_valid = false;
}

/*****************************************************************************/
/*                      jpx_roi_editor::get_bounding_box                     */
/*****************************************************************************/

bool jpx_roi_editor::get_bounding_box(kdu_dims &bb,
                                      bool include_scribble) const
{
  if (num_regions <= 0)
    return false;
  int n;
  kdu_dims result;
  for (n=0; n < num_regions; n++)
    { 
      result.augment(regions[n].region);
      for (int p=0; p < 4; p++)
        result.augment(regions[n].vertices[p]);
    }
  if (include_scribble)
    for (n=0; n < num_scribble_points; n++)
      result.augment(scribble_points[n]);
  bb = result;
  return true;
}

/*****************************************************************************/
/*                          jpx_roi_editor::set_mode                         */
/*****************************************************************************/

kdu_dims jpx_roi_editor::set_mode(jpx_roi_editor_mode mode)
{
  kdu_dims result;
  if (mode == this->mode)
    return result;
  result = cancel_selection();
  this->mode = mode;
  kdu_dims bb; get_bounding_box(bb,false);
  if (!bb.is_empty())
    result.augment(bb);
  return result;
}
  
/*****************************************************************************/
/*                     jpx_roi_editor::find_nearest_anchor                   */
/*****************************************************************************/

bool
  jpx_roi_editor::find_nearest_anchor(kdu_coords &point,
                                      bool modify_for_selection) const
{
  if (num_regions == 0)
    return false;
  kdu_coords ref=point;
  kdu_long min_dist=-1;
  for (int n=0; n < num_regions; n++)
    {
      kdu_coords v[5];
      int p, num_anchors = find_anchors(v,regions[n]);
      if ((num_anchors == 4) && regions[n].is_elliptical)
        { // Add the ellipse centre as a point to be searched
          kdu_coords extent, skew;
          regions[n].get_ellipse(v[4],extent,skew);
          num_anchors = 5;
        }
      for (p=0; p < num_anchors; p++)
        {
          if ((n == region_idx) && (p == anchor_idx) && modify_for_selection)
            continue;
          kdu_long dx=v[p].x-ref.x, dy=v[p].y-ref.y;
          kdu_long dist = dx*dx + dy*dy;
          if ((min_dist < 0) || (dist < min_dist))
            {
              point = v[p];
              min_dist = dist;
            }
        }
    }
  return true;
}

/*****************************************************************************/
/*                jpx_roi_editor::find_nearest_boundary_point                */
/*****************************************************************************/

bool
  jpx_roi_editor::find_nearest_boundary_point(kdu_coords &point,
                                              bool exclude_sel_region) const
{
  if (num_regions == 0)
    return false;
  double x0=point.x, y0=point.y;
  double nearest_xp=x0, nearest_yp=y0;
  kdu_dims bb; get_bounding_box(bb,false);
  double min_distance = (bb.size.x>bb.size.y)?bb.size.x:bb.size.y;
  bool have_something = false;
  for (int n=0; n < num_regions; n++)
    { 
      if (exclude_sel_region && (n == region_idx))
        continue;
      double xp=x0, yp=y0;
      if (regions[n].find_boundary_projection(x0,y0,xp,yp,min_distance) < 0)
        continue;
      have_something = true;
      double dx = xp-x0, dy = yp-y0;
      double distance = sqrt(dx*dx + dy*dy);
      assert(distance <= min_distance);
      min_distance = distance;
      nearest_xp = xp;
      nearest_yp = yp;
    }
  point.x = (int) floor(0.5+nearest_xp);
  point.y = (int) floor(0.5+nearest_yp);
  return have_something;
}

/*****************************************************************************/
/*                 jpx_roi_editor::find_nearest_guide_point                  */
/*****************************************************************************/

bool
  jpx_roi_editor::find_nearest_guide_point(kdu_coords &point) const
{
  if ((region_idx < 0) || (region_idx >= num_regions) || (anchor_idx < 0))
    return false;
  const jpx_roi *roi = regions+region_idx;

  if (mode == JPX_EDITOR_VERTEX_MODE)
    { // Vertex mode
      if (roi->is_elliptical)
        { 
          kdu_coords test = point;
          if (jx_project_to_line(roi->vertices[anchor_idx],
                                 roi->vertices[(anchor_idx+2)&3],test) &&
              (test != anchor_point))
            { point = test; return true; }
          else
            return false;
        }
      
      // If we get here, we are considering a quadrilateral region's guidelines
      kdu_long dx, dy, dist_sq, min_dist_sq=-1;
      kdu_coords src = point;
      for (int e=0; e < 2; e++)
        { // Candidates are the edges running through the anchor point
          kdu_coords test=src;
          if (!jx_project_to_line(roi->vertices[anchor_idx],
                                  roi->vertices[(anchor_idx+2*e-1)&3],test))
            continue;
          if (test == anchor_point)
            continue;
          dx = test.x - (kdu_long) src.x;  dy = test.y - (kdu_long) src.y;
          dist_sq = dx*dx + dy*dy;
          if ((min_dist_sq < 0) || (dist_sq < min_dist_sq))
            { point = test; min_dist_sq = dist_sq; }
        }
      return (min_dist_sq >= 0);
    }
  else
    { // Skeleton and path modes are very similar
      if (roi->is_elliptical)
        return false;

      // If we get here, we are considering a quadrilateral region's guidelines
      kdu_long dx, dy, dist_sq, min_dist_sq=-1;
      kdu_coords src = point;
      for (int e=0; e < 3; e++)
        { // Three candidate guidelines
          if ((e != 1) && (mode != JPX_EDITOR_SKELETON_MODE))
            continue; // Path mode has only one guideline, rather than 3
          kdu_coords from = roi->vertices[(anchor_idx-1+e)&3];
          kdu_coords to = roi->vertices[(anchor_idx+e)&3];
          kdu_coords disp; // Displacement for edge to make it run thru anchor
          if (e == 0)
            disp = anchor_point - to;
          else if (e == 2)
            disp = anchor_point - from;
          from += disp; to += disp;
          kdu_coords test=src;
          if (!jx_project_to_line(from,to,test))
            continue;
          if (test == anchor_point)
            continue;
          dx = test.x - (kdu_long) src.x;  dy = test.y - (kdu_long) src.y;
          dist_sq = dx*dx + dy*dy;
          if ((min_dist_sq < 0) || (dist_sq < min_dist_sq))
            { point = test; min_dist_sq = dist_sq; }          
        }
      return (min_dist_sq >= 0);
    }
  
  return false;
}

/*****************************************************************************/
/*                       jpx_roi_editor::select_anchor                       */
/*****************************************************************************/

kdu_dims
  jpx_roi_editor::select_anchor(kdu_coords point, bool advance)
{
  kdu_dims result;
  int n, p, num_anchors;
  kdu_coords anchors[4];
  
  // First figure out what to do with an existing selection
  if ((anchor_idx >= 0) && (region_idx >= 0) && (region_idx < num_regions))
    {
      num_anchors = find_anchors(anchors,regions[region_idx]);
      if ((anchor_idx < num_anchors) && (anchors[anchor_idx] == point))
        {
          if (!advance)
            {
              anchor_point = point;
              return result; // No change, leave selection exactly as is
            }
        }
      else
        result = cancel_selection();
    }
  else
    result = cancel_selection();

  if (anchor_idx < 0)
    memset(drag_flags,0,(size_t) num_regions);
  else
    {
      result = cancel_drag();
      anchor_point = dragged_point = point;
      jpx_roi *roi = regions+region_idx;
      kdu_dims edge_region = get_edge_region(roi,edge_idx);
      result.augment(edge_region);
    }
  assert(dragged_point == anchor_point);

  // At this point, we are either finding a new anchor point from scratch
  // (`anchor_idx' < 0), or we are advancing an existing anchor selection
  // (identified by `anchor_idx', `region_idx' and `edge_idx').
  
  bool trying_to_advance = (anchor_idx >= 0);
  for (n=0; n < num_regions; n++)
    {
      num_anchors = find_anchors(anchors,regions[n]);
      jpx_roi *roi = regions+n;
      for (p=0; p < num_anchors; p++)
        {
          if (point == anchors[p])
            {
              result.augment(point);
              if (anchor_idx < 0)
                {
                  anchor_idx = p;
                  region_idx = n;
                  edge_idx = find_next_anchor_edge();
                }
              else if ((anchor_idx == p) && (region_idx == n))
                {
                  if ((edge_idx = find_next_anchor_edge()) < 0)
                    { 
                      anchor_idx = region_idx = -1;
                      memset(drag_flags,0,(size_t) num_regions);
                      continue;
                    }
                }
              else
                continue;

              anchor_point = dragged_point = point;
              if (mode == JPX_EDITOR_VERTEX_MODE)
                { // Only the anchor point moves
                  drag_flags[n] |= (kdu_byte)(1<<p);
                  if (!roi->is_elliptical)
                    set_drag_flags_for_vertex(point);
                }
              else if (roi->is_elliptical)
                { // The entire ellipse moves
                  drag_flags[n] |= 0x0F;
                  set_drag_flags_for_boundary(roi);
                }
              else
                { // The vertices defining the edge containing the anchor move
                  drag_flags[n] |= (kdu_byte)((1<<p) | (1<<((p+1)&3)));
                  set_drag_flags_for_vertex(roi->vertices[p]);
                  set_drag_flags_for_vertex(roi->vertices[(p+1)&3]);
                  set_drag_flags_for_midpoint(point);
                }
              kdu_dims edge_region = get_edge_region(roi,edge_idx);
              result.augment(edge_region);
              return result;
            }
        }
    }
  if ((anchor_idx < 0) && trying_to_advance)
    { // Give it another go
      kdu_dims second_result = select_anchor(point,false);
      if (!second_result.is_empty())
        result.augment(second_result);
    }
  return result;
}

/*****************************************************************************/
/*                      jpx_roi_editor::cancel_selection                     */
/*****************************************************************************/

kdu_dims
  jpx_roi_editor::cancel_selection()
{
  kdu_dims result = cancel_drag();
  if ((region_idx >= 0) && (region_idx < num_regions) &&
      (anchor_idx >= 0) && (anchor_idx < 4))
    { 
      jpx_roi *sel_roi = regions + region_idx;
      result.augment(anchor_point);
      if (sel_roi->is_elliptical)
        { 
          update_extremities(sel_roi);
          result.augment(sel_roi->region);
        }
      else
        { 
          kdu_coords from, to;
          get_edge_vertices(sel_roi,edge_idx,from,to);
          result.augment(from);
          result.augment(to);
        }
    }
  region_idx = edge_idx = anchor_idx = -1;
  anchor_point = dragged_point = kdu_coords();
  memset(drag_flags,0,(size_t) num_regions);
  return result;
}

/*****************************************************************************/
/*                    jpx_roi_editor::drag_selected_anchor                  */
/*****************************************************************************/

kdu_dims
  jpx_roi_editor::drag_selected_anchor(kdu_coords new_point)
{
  if ((region_idx < 0) || (anchor_idx < 0) || (region_idx >= num_regions))
    return kdu_dims();
  if (new_point == dragged_point)
    return kdu_dims();
    
  kdu_dims result = cancel_drag();
  dragged_point = new_point;
  kdu_dims new_region = cancel_drag(); // Useful way to find affected region
  dragged_point = new_point;
  if (!new_region.is_empty())
    result.augment(new_region);
  return result;
}

/*****************************************************************************/
/*                  jpx_roi_editor::can_move_selected_anchor                 */
/*****************************************************************************/

bool
  jpx_roi_editor::can_move_selected_anchor(kdu_coords new_point,
                                           bool check_roid_limit) const
{
  if ((anchor_idx < 0) || (anchor_idx >= 4) ||
      (region_idx < 0) || (region_idx >= num_regions))
    return false;

  kdu_coords disp = new_point - anchor_point;
  if ((disp.x == 0) && (disp.y == 0))
    return false;
  if (regions[region_idx].is_elliptical && (mode == JPX_EDITOR_VERTEX_MODE))
    { // Displacement needs to be even to be sure of having an effect, since
      // ellipse locations are determined by the centres.
      disp.x = (disp.x>0)?(disp.x+(disp.x&1)):(disp.x-(disp.x&1));
      disp.y = (disp.y>0)?(disp.y+(disp.y&1)):(disp.y-(disp.y&1));
      new_point = anchor_point + disp;
    }

  int num_roid_elts = 0;
  for (int n=0; n < num_regions; n++)
    { 
      if (drag_flags[n] != 0)
        { 
          jpx_roi test_roi = regions[n];
          move_vertices(&test_roi,drag_flags[n],disp);
          if (!test_roi.check_geometry())
            return false; // Failed to create a valid region
          num_roid_elts += (test_roi.is_simple())?1:2;
        }
      else
        num_roid_elts += (regions[n].is_simple())?1:2;
    }
  if (check_roid_limit && (num_roid_elts > 255))
    return false;
  return true;
}

/*****************************************************************************/
/*                    jpx_roi_editor::move_selected_anchor                   */
/*****************************************************************************/

kdu_dims
  jpx_roi_editor::move_selected_anchor(kdu_coords new_point)
{
  kdu_dims result = cancel_drag();
  if (!can_move_selected_anchor(new_point,false))
    return result;
  if (!can_move_selected_anchor(new_point,true))
    { KDU_WARNING(w,0x09031001); w <<
      KDU_TXT("ROI shape edit will cause the maximum number of regions in "
              "the JPX ROI Description box to be exceeded.  Delete or "
              "simplify some regions to simple rectangles or ellipses.");
      return result;
    }
  
  kdu_coords disp = new_point - anchor_point;
  if ((disp.x == 0) && (disp.y == 0))
    return result;
  if (regions[region_idx].is_elliptical && (mode == JPX_EDITOR_VERTEX_MODE))
    { // Displacement needs to be even to be sure of having an effect, since
      // ellipse locations are determined by the centres.
      disp.x = (disp.x>0)?(disp.x+(disp.x&1)):(disp.x-(disp.x&1));
      disp.y = (disp.y>0)?(disp.y+(disp.y&1)):(disp.y-(disp.y&1));
      new_point = anchor_point + disp;
    }
  push_current_state();
  
  // Now perform the move itself
  int n;
  kdu_dims existing_bb; get_bounding_box(existing_bb,false);
  result.augment(existing_bb);
  
  // Now consider all the other regions
  for (n=0; n < num_regions; n++)
    if (drag_flags[n] != 0)
      move_vertices(regions+n,drag_flags[n],disp);
  kdu_dims new_bb;
  get_bounding_box(new_bb,false);
  result.augment(new_bb);

  find_nearest_anchor(new_point,false);
  select_anchor(new_point,true);
  dragged_point = anchor_point;
  path_edge_flags_valid = shared_edge_flags_valid = false;
  
  result.augment(remove_duplicates());
  return result;
}

/*****************************************************************************/
/*                        jpx_roi_editor::cancel_drag                        */
/*****************************************************************************/

kdu_dims
  jpx_roi_editor::cancel_drag()
{
  if ((anchor_idx < 0) || (anchor_idx >= 4) ||
      (region_idx < 0) || (region_idx >= num_regions) ||
      (anchor_point == dragged_point))
    return kdu_dims(); // Nothing to do; no drag happening
  kdu_coords disp = dragged_point - anchor_point;
  kdu_dims result; result.pos = dragged_point; result.size = kdu_coords(1,1);
  
  int n, p;
  kdu_byte mask;
  for (n=0; n < num_regions; n++)
    {
      if (drag_flags[n] == 0)
        continue; // No points of this region affected by dragging
      if (regions[n].is_elliptical)
        { // Entire dragged ellipse needs to be included in redraw region
          jpx_roi roi_copy = regions[n];
          if (drag_flags[n] != 0)
            move_vertices(&roi_copy,drag_flags[n],disp);
          result.augment(roi_copy.region);
        }
      else
        { // Only the dragged edges need to be included
          for (p=0, mask=1; p < 4; p++, mask<<=1)
            if (drag_flags[n] & mask)
              {
                result.augment(regions[n].vertices[p] + disp);
                for (int i=-1; i <= 1; i+=2)
                  { 
                    int ep = (p+i) & 3;
                    kdu_coords end_point = regions[n].vertices[ep];
                    if (drag_flags[n] & (kdu_byte)(1<<ep))
                      end_point += disp;
                    result.augment(end_point);
                  }
              }
        }
    }
  
  dragged_point = anchor_point;
  return result;
}

/*****************************************************************************/
/*                       jpx_roi_editor::get_selection                       */
/*****************************************************************************/

int jpx_roi_editor::get_selection(kdu_coords &point, int &num_instances) const
{
  if ((region_idx < 0) || (region_idx >= num_regions) || (anchor_idx < 0))
    return -1;
  point = this->anchor_point;
  num_instances = 0;
  int n, p;
  for (n=0; n < num_regions; n++)
    {
      kdu_coords anchors[4];
      int num_anchors = find_anchors(anchors,regions[n]);
      for (p=0; p < num_anchors; p++)
        if (anchors[p] == point)
          num_instances++;
    }
  return region_idx;
}

/*****************************************************************************/
/*                   jpx_roi_editor::split_selected_anchor                   */
/*****************************************************************************/

kdu_dims jpx_roi_editor::split_selected_anchor()
{
  kdu_dims result;
  if ((anchor_idx < 0) || (region_idx < 0) || (region_idx >= num_regions))
    return result;
  
  push_current_state();
  
  kdu_coords disp, v[4];
  int n, p, num_anchors, trial;
  bool unique = false;
  for (trial=0; (trial < 9) && !unique; trial++)
    { 
      jpx_roi *sel_roi = regions+region_idx;
      switch (trial % 3) {
        case 0: disp.x=0; break;
        case 1: disp.x=1; break;
        case 2: disp.x=-1; break;
      };
      switch ((trial / 3) % 3) {
        case 0: disp.y=0; break;
        case 1: disp.y=1; break;
        case 2: disp.y=-1; break;
      };
      
      kdu_coords new_point = anchor_point + disp;
      for (unique=true, n=0; (n < num_regions) && unique; n++)
        { 
          num_anchors = find_anchors(v,regions[n]);
          for (p=0; p < num_anchors; p++)
            if ((v[p] == new_point) &&
                ((p != anchor_idx) || (n != region_idx)))
              break;
          if (p < num_anchors)
            unique = false;
        }
      if (unique && (disp.x || disp.y))
        { 
          p = anchor_idx;
          jpx_roi new_roi = *sel_roi;
          kdu_byte dflags = 0;
          if (new_roi.is_elliptical)
            dflags = 0x0F; // Move the entire ellipse -- only safe action
          else if (mode == JPX_EDITOR_VERTEX_MODE)
            dflags = (kdu_byte)(1<<p);
          else
            dflags = (kdu_byte)((1<<p) | (1<<((p+1)&3)));
          move_vertices(&new_roi,dflags,disp);
          if (!new_roi.check_geometry())
            { unique = false; continue; }
          int save_edge_idx = edge_idx;
          result = cancel_selection();
          *sel_roi = new_roi;
          select_anchor(new_point,false);
          if (edge_idx != save_edge_idx)
            select_anchor(new_point,true);
          if (edge_idx != save_edge_idx)
            select_anchor(new_point,false);
          result.augment(sel_roi->region);
        }
    }
  
  path_edge_flags_valid = shared_edge_flags_valid = false;
  return result;
}

/*****************************************************************************/
/*                    jpx_roi_editor::clear_scribble_points                  */
/*****************************************************************************/

kdu_dims jpx_roi_editor::clear_scribble_points()
{
  if (num_scribble_points != 0)
    push_current_state();
  kdu_dims result;
  for (int n=0; n < num_scribble_points; n++)
    result.augment(scribble_points[n]);
  num_scribble_points = num_subsampled_scribble_points = 0;
  return result;
}

/*****************************************************************************/
/*                      jpx_roi_editor::add_scribble_point                   */
/*****************************************************************************/

kdu_dims jpx_roi_editor::add_scribble_point(kdu_coords point)
{
  kdu_dims result;
  
  // First see if we have to sub-sample the existing points
  if (num_scribble_points == JX_ROI_SCRIBBLE_POINTS)
    {
      if (num_subsampled_scribble_points >= (num_scribble_points-1))
        num_subsampled_scribble_points = 0; // Start sub-sampling from scratch
      int p, n=num_subsampled_scribble_points-1;;
      result.augment(scribble_points[n-1]);
      for (; n < num_scribble_points; n++)
        { // Remove the point at location `n'
          result.augment(scribble_points[n]);
          num_scribble_points--;
          for (p=n; p < num_scribble_points; p++)
            scribble_points[p] = scribble_points[p+1];
        }
      num_subsampled_scribble_points = num_scribble_points;
    }
  
  // Now look for loops to eliminate;
  bool possible_loops = true;
  while ((num_scribble_points > 1) && possible_loops)
    { 
      kdu_coords last_point = scribble_points[num_scribble_points-1];
      result.augment(last_point);
      if (point == last_point)
        return result;
      
      // Let line segment A->B be defined by `point' and `last_point', and
      // scan through all the other line segments, denoting them C->D.
      // Note that the last line segment scanned has D = B, but it is
      // sufficient to check for an intersection between the half-open
      // line segments [A,B) and [C,D], since the `last_point' B was
      // checked for intersection when it was added.  This ensures that
      // we never misdetect an intersection between the new line segment
      // and the last one.
      int p, n;
      for (n=0; n < (num_scribble_points-1); n++)
        if (jx_check_line_intersection(point,last_point,false,true,
                                       scribble_points[n],
                                       scribble_points[n+1]))
          break;
      if (n == (num_scribble_points-1))
        possible_loops = false;
      else
        { // Found a loop
          if (n > (num_scribble_points/2))
            { // Eliminate everything from point n+1 to the end
              for (p=n+1; p < num_scribble_points; p++)
                result.augment(scribble_points[p]);
              num_scribble_points = n+1;
            }
          else
            { // Eliminate everything up to point n.
              for (p=0; p <= n; p++)
                result.augment(scribble_points[p]);
              for (n++; p < num_scribble_points; p++)
                scribble_points[p-n] = scribble_points[p];
              num_scribble_points -= n;
            }
        }
    }
  
  result.augment(point);
  scribble_points[num_scribble_points++] = point;
  return result;
}

/*****************************************************************************/
/*                   jpx_roi_editor::convert_scribble_path                   */
/*****************************************************************************/

kdu_dims jpx_roi_editor::convert_scribble_path(bool replace_content,
                                               int conversion_flags,
                                               double accuracy)
{
  if (num_scribble_points <= 1)
    return kdu_dims();
  
  // First convert `accuracy' into a reasonable `max_sq_dist' value
  int n;
  kdu_long max_sq_dist = 1;
  if (accuracy < 1.0)
    { 
      kdu_coords min=scribble_points[0], max=min;
      for (n=1; n < num_scribble_points; n++)
        { 
          if (scribble_points[n].x < min.x)
            min.x = scribble_points[n].x;
          else if (scribble_points[n].x > max.x)
            max.x = scribble_points[n].x;
          if (scribble_points[n].y < min.y)
            min.y = scribble_points[n].y;
          else if (scribble_points[n].y > max.y)
            max.y = scribble_points[n].y;
        }
      kdu_coords red_size = max-min;
      red_size.x = (red_size.x+3)>>2;  red_size.y = (red_size.y+3)>>2;
      max_sq_dist = red_size.x;  max_sq_dist *= red_size.y;
      if (accuracy > 0.0)
        { 
          double max_val = (double) max_sq_dist;
          double scale = exp(accuracy * log(max_val));
          max_sq_dist = (kdu_long)(0.5 + max_val/scale);
        }
    }
  
  jx_scribble_converter *converter = new jx_scribble_converter;
  bool want_fill = ((conversion_flags & JPX_EDITOR_FLAG_FILL) != 0);
  if (want_fill && (num_scribble_points > 2) &&
      (scribble_points[0] != scribble_points[num_scribble_points-1]))
    { // Temporarily backup the scribble points, so we can close the loop
      // before initializing the converter
      int npoints = num_scribble_points;
      kdu_coords *backup = new kdu_coords[npoints];
      memcpy(backup,scribble_points,sizeof(kdu_coords)*((size_t) npoints));
      add_scribble_point(scribble_points[0]);
      converter->init(scribble_points,num_scribble_points,want_fill);
      num_scribble_points = npoints;
      memcpy(scribble_points,backup,sizeof(kdu_coords)*((size_t) npoints));
      delete[] backup;
    }
  else
    converter->init(scribble_points,num_scribble_points,want_fill);
  
  jx_path_filler *path_filler = NULL;
  try {
    if (!converter->find_polygon_edges(max_sq_dist))
      throw ((int) 0);
    if (!converter->find_boundary_vertices())
      throw ((int) 0);
    int needed_regions = 0;
    if (want_fill)
      { 
        path_filler = new jx_path_filler;
        if (!path_filler->init(converter->boundary_vertices,
                               converter->num_boundary_vertices))
          throw ((int) 0);
        if (!path_filler->process())
          { KDU_WARNING(w,0x05051001); w <<
            KDU_TXT("Unable to completely fill the region bounded by the "
                    "scribble path; this is probably an internal flaw; "
                    "you could either fill the unfilled portion manually "
                    "or try again with a different complexity setting.");
          }
        needed_regions = path_filler->num_regions;
      }
    else
      { 
        needed_regions = (converter->num_boundary_vertices-1);
        if (conversion_flags & JPX_EDITOR_FLAG_ELLIPSES)
          needed_regions += converter->num_boundary_vertices;
      }
    if ((needed_regions < 1) || (needed_regions > 255) ||
        (replace_content && (needed_regions > (255-this->num_regions))))
      throw ((int) 0);
  } catch (...) {
    if (converter != NULL)
      delete converter;
    if (path_filler != NULL)
      delete path_filler;
    return kdu_dims();
  }

  // If we get here, we are committed to making the changes
  push_current_state();
  kdu_dims result;
  if (replace_content)    
    { 
      get_bounding_box(result,false);
      result.augment(cancel_selection());
      num_regions = 0;
    }
  
  path_edge_flags_valid = shared_edge_flags_valid = false;
  if (path_filler != NULL)
    { 
      for (n=0; n < path_filler->num_regions; n++, num_regions++)
        { 
          assert(num_regions < 255);
          jpx_roi *roi = regions + num_regions;
          kdu_coords *vertices = path_filler->region_vertices + 4*n;
          roi->init_quadrilateral(vertices[0],vertices[1],
                                  vertices[2],vertices[3]);
          update_extremities(roi);  
          result.augment(roi->region);
        }
      delete path_filler;
    }
  else
    { // Generate a path formed by degenerate quadrilateral line segments
      jpx_roi *roi;
      for (n=1; n < converter->num_boundary_vertices; n++)
        {
          assert(num_regions < 255);
          roi = regions + (num_regions++);
          roi->init_quadrilateral(converter->boundary_vertices[n-1],
                                  converter->boundary_vertices[n],
                                  converter->boundary_vertices[n],
                                  converter->boundary_vertices[n-1]);
          update_extremities(roi);
          result.augment(roi->region);
        }
      if (conversion_flags & JPX_EDITOR_FLAG_ELLIPSES)
        { 
          for (n=0; n < converter->num_boundary_vertices; n++)
            { 
              assert(num_regions < 255);
              roi = regions + (num_regions++);
              roi->init_ellipse(converter->boundary_vertices[n],
                                kdu_coords(1,1));
              update_extremities(roi);
              result.augment(roi->region);
            }
        }
    }

  delete converter;
  return result;
}

/*****************************************************************************/
/*                         jpx_roi_editor::get_anchor                        */
/*****************************************************************************/

int jpx_roi_editor::get_anchor(kdu_coords &pt, int which,
                               bool selected_region_only, bool dragged) const
{
  if ((which < 0) ||
      (dragged && ((anchor_idx < 0) || (dragged_point == anchor_point))))
    return 0;
  int n=0, lim_n=num_regions;
  if (selected_region_only || dragged)
    {
      if ((region_idx < 0) || (region_idx >= num_regions))
        return 0;
      n = region_idx; lim_n = n+1;
    }
  for (; n < lim_n; n++)
    { 
      kdu_coords anchors[4];
      int num_anchors = find_anchors(anchors,regions[n]);
      if (dragged)
        {
          if (which != 0)
            return 0;
          which = anchor_idx;
        }
      else if (which >= num_anchors)
        { 
          which -= num_anchors;
          continue;
        }
      int flags = JPX_EDITOR_FLAG_NZ;
      if ((n == region_idx) && (which == anchor_idx))
        flags |= JPX_EDITOR_FLAG_SELECTED;
      if (regions[n].is_encoded)
        flags |= JPX_EDITOR_FLAG_ENCODED;
      pt = anchors[which];
      if (dragged)
        pt += (dragged_point - anchor_point);
      return flags;
    }
  return 0;
}

/*****************************************************************************/
/*                          jpx_roi_editor::get_edge                         */
/*****************************************************************************/

int jpx_roi_editor::get_edge(kdu_coords &from, kdu_coords &to, int which,
                             bool selected_region_only, bool dragged,
                             bool want_shared_flag)
{
  if ((which < 0) ||
      (dragged && ((anchor_idx < 0) || (dragged_point == anchor_point))))
    return 0;
  if (want_shared_flag && !shared_edge_flags_valid)
    find_shared_edge_flags();
  kdu_coords disp = dragged_point - anchor_point;
  int n=0, lim_n=num_regions;
  if (selected_region_only)
    { 
      if ((region_idx < 0) || (region_idx >= num_regions))
        return 0;
      n = region_idx; lim_n = n+1;
    }
  for (; n < lim_n; n++)
    { 
      if (dragged && (drag_flags[n] == 0))
        continue;
      int flags = JPX_EDITOR_FLAG_NZ;
      if (regions[n].is_encoded)
        flags |= JPX_EDITOR_FLAG_ENCODED;
      if (regions[n].is_elliptical)
        { 
          if (which >= 2)
            { which-=2; continue; }
          jpx_roi roi_copy = regions[n];
          if (dragged)
            move_vertices(&roi_copy,drag_flags[n],disp);
          get_edge_vertices(&roi_copy,(which+1),from,to);
          if ((n == region_idx) && (edge_idx == (which+1)))
            flags |= JPX_EDITOR_FLAG_SELECTED;
        }
      else
        {
          if (dragged)
            {
              int p;
              for (p=0; p < 4; p++)
                {
                  kdu_byte mask1 = (kdu_byte)(1<<p);
                  kdu_byte mask2 = (p==3)?((kdu_byte) 1):(mask1<<1);
                  if (!(drag_flags[n] & (mask1 | mask2)))
                    continue;
                  if (which > 0)
                    { which--; continue; }
                  from = regions[n].vertices[p];
                  to = regions[n].vertices[(p+1)&3];
                  if ((n == region_idx) && (p == edge_idx))
                    flags |= JPX_EDITOR_FLAG_SELECTED;
                  if (want_shared_flag && (shared_edge_flags[n] & mask1))
                    flags |= JPX_EDITOR_FLAG_SHARED;
                  if (drag_flags[n] & mask1)
                    from += disp;
                  if (drag_flags[n] & mask2)
                    to += disp;
                  break;
                }
              if (p == 4)
                continue;
            }
          else
            { 
              if (which >= 4)
                { which -= 4; continue; }
              if ((n == region_idx) && (which == edge_idx))
                flags |= JPX_EDITOR_FLAG_SELECTED;
              if (want_shared_flag &&
                  (shared_edge_flags[n] & ((kdu_byte)(1<<which))))
                flags |= JPX_EDITOR_FLAG_SHARED;
              from = regions[n].vertices[which];
              to = regions[n].vertices[(which+1)&3];
              if (which == edge_idx)
                flags |= JPX_EDITOR_FLAG_SELECTED;
            }
        }
      return flags;
    }
  return 0;
}

/*****************************************************************************/
/*                        jpx_roi_editor::get_curve                          */
/*****************************************************************************/

int jpx_roi_editor::get_curve(kdu_coords &centre, kdu_coords &extent,
                              kdu_coords &skew, int which,
                              bool selected_region_only, bool dragged) const
{
  if ((which < 0) ||
      (dragged && ((anchor_idx < 0) || (dragged_point == anchor_point))))
    return 0;
  kdu_coords disp = dragged_point - anchor_point;
  int n=0, lim_n=num_regions;
  if (selected_region_only)
    { 
      if ((region_idx < 0) || (region_idx >= num_regions))
        return 0;
      n = region_idx; lim_n = n+1;
    }
  for (; n < lim_n; n++)
    { 
      if (!regions[n].is_elliptical)
        continue;
      if (dragged && (drag_flags[n] == 0))
        continue;
      if (which > 0)
        { which--; continue; }
      if (!dragged)
        regions[n].get_ellipse(centre,extent,skew);
      else
        {
          jpx_roi roi_copy = regions[n];
          move_vertices(&roi_copy,drag_flags[n],disp);
          roi_copy.get_ellipse(centre,extent,skew);
        }
      int flags = JPX_EDITOR_FLAG_NZ;
      if (regions[n].is_encoded)
        flags |= JPX_EDITOR_FLAG_ENCODED;
      if ((n == region_idx) && (edge_idx ==  0))
        flags |= JPX_EDITOR_FLAG_SELECTED;
      return flags;
    }
  return 0;
}

/*****************************************************************************/
/*                 jpx_roi_editor::get_path_segment_for_region               */
/*****************************************************************************/

bool jpx_roi_editor::get_path_segment_for_region(int idx, kdu_coords &from,
                                                 kdu_coords &to)
{
  if ((idx < 0) || (idx >= num_regions))
    return false;
  if (!path_edge_flags_valid)
    find_path_edge_flags();
  kdu_byte edges = path_edge_flags[idx];
  kdu_byte type = (edges>>6);  edges &= 0x0F;
  if (type == 0)
    return false;
  jpx_roi *roi = regions + idx;
  if (type == 3)
    {
      assert(roi->is_elliptical);
      from.x = to.x = roi->region.pos.x + (roi->region.size.x>>1);
      from.y = to.y = roi->region.pos.y + (roi->region.size.y>>1);
    }
  else if (type == 1)
    {
      from = jx_midpoint(roi->vertices[0],roi->vertices[1]);
      to = jx_midpoint(roi->vertices[2],roi->vertices[3]);
    }
  else if (type == 2)
    {
      from = jx_midpoint(roi->vertices[1],roi->vertices[2]);
      to = jx_midpoint(roi->vertices[3],roi->vertices[0]);
    }
  return true;
}

/*****************************************************************************/
/*                      jpx_roi_editor::get_path_segment                     */
/*****************************************************************************/

int jpx_roi_editor::get_path_segment(kdu_coords &from, kdu_coords &to,
                                     int which)
{
  if (which <  0)
    return 0;
  if (!path_edge_flags_valid)
    find_path_edge_flags();
  
  int n;
  for (n=0; n < num_regions; n++)
    { 
      jpx_roi *roi = regions + n;
      if (roi->is_elliptical)
        continue;
      kdu_byte edges = path_edge_flags[n];
      kdu_byte type = (edges>>6);  edges &= 0x0F;
      if (type == 1)
        { // This region is a path brick running from edge 0 to edge 2
          if (which > 0)
            { which--; continue; }
          from = jx_midpoint(roi->vertices[0],roi->vertices[1]);
          to = jx_midpoint(roi->vertices[2],roi->vertices[3]);
          return JPX_EDITOR_FLAG_NZ;
        }
      else if (type == 2)
        { // This region is a path brick running from edge 1 to edge 3
          if (which > 0)
            { which--; continue; }
          from = jx_midpoint(roi->vertices[1],roi->vertices[2]);
          to = jx_midpoint(roi->vertices[3],roi->vertices[0]);
          return JPX_EDITOR_FLAG_NZ;
        }
    }
  return 0;
}

/*****************************************************************************/
/*                         jpx_roi_editor::enum_paths                        */
/*****************************************************************************/

int jpx_roi_editor::enum_paths(kdu_uint32 path_flags[],
                               kdu_byte path_members[],
                               kdu_coords &path_start, kdu_coords &path_end)
{
  if (!path_edge_flags_valid)
    find_path_edge_flags();
  int n, m, num_members=0;
  while (num_members < 255)
    { // Loop until we cannot add any more segments on
      
      // Start by looking for a matching path brick; but we need to be
      // careful to stop if we encounter a junction.
      kdu_uint32 mask, found_mask, *pflags, *found_pflags=NULL;
      kdu_coords ep1, ep2, found_ep1, found_ep2;
      int found_brick_idx = -1;
      bool add_to_tail = false;
      for (mask=1, pflags=path_flags, n=0;
           n < num_regions; n++, mask<<=1)
        { 
          if (mask == 0) { mask=1; pflags++; }
          if ((*pflags & mask) || regions[n].is_elliptical ||
              !get_path_segment_for_region(n,ep1,ep2))
            continue;
          if ((num_members == 0) || (ep1 == path_end) || (ep2 == path_end) ||
              (ep1 == path_start) || (ep2 == path_start))
            { 
              if (found_brick_idx < 0)
                { 
                  found_brick_idx = n;
                  found_pflags=pflags;  found_mask=mask;
                  found_ep1=ep1;  found_ep2=ep2;
                  if (num_members == 0)
                    break; // No need to check for junctions yet
                  add_to_tail = (ep1 == path_end) || (ep2 == path_end);
                }
              else if ((add_to_tail &&
                        ((ep1==path_end) || (ep2==path_end))) ||
                       ((!add_to_tail) &&
                        ((ep1==path_start) || (ep2==path_start))))
                { // Junction encountered
                  found_pflags = NULL;
                  break;
                }
            }
        }
      
      if ((found_brick_idx < 0) || (found_pflags == NULL))
        break; // We're all done
          
      *found_pflags |= found_mask;
      if (add_to_tail)
        { 
          for (m=num_members++; m > 0; m--)
            path_members[m] = path_members[m-1];
          path_members[0] = (kdu_byte) found_brick_idx;
          path_end = (found_ep1==path_end)?found_ep2:found_ep1;
        }
      else
        { 
          if (num_members == 0)
            { path_start = found_ep1; path_end = found_ep2; }
          else
            path_start = (found_ep1==path_start)?found_ep2:found_ep1;
          path_members[num_members++] = (kdu_byte) found_brick_idx;
        }
          
      // Finish off this iteration by looking for ellipses
      for (mask=1, pflags=path_flags, n=0;
           (n < num_regions) && (num_members < 255);
           n++, mask<<=1)
        { 
          if (mask == 0) { mask=1; pflags++; }
          if ((*pflags & mask) || ((path_edge_flags[n] >> 6) != 3) ||
              !regions[n].is_elliptical)
            continue;
          kdu_coords centre = regions[n].region.pos;
          centre.x += regions[n].region.size.x >> 1;
          centre.y += regions[n].region.size.y >> 1;
          if (centre == path_start)
            { 
              for (m=num_members++; m > 0; m--)
                path_members[m] = path_members[m-1];
              path_members[0] = (kdu_byte) n;
              *pflags |= mask;
            }
          else if (centre == path_end)
            { 
              path_members[num_members++] = (kdu_byte) n;
              *pflags |= mask;
            }
        }
    }
  return num_members;
}

/*****************************************************************************/
/*                    jpx_roi_editor::fill_closed_paths                      */
/*****************************************************************************/

kdu_dims
  jpx_roi_editor::fill_closed_paths(bool &success, int required_member_idx)
{
  // Start by collecting the paths
  jx_path_filler *scan, *head=NULL;
  
  kdu_byte path_members[256];
  kdu_uint32 path_flags[8]={0,0,0,0,0,0,0,0};
  kdu_coords path_start, path_end;
  int num_members, n;
  while ((num_members=enum_paths(path_flags,path_members,
                                 path_start,path_end)) > 0)
    { 
      if (path_start != path_end)
        continue;
      if (required_member_idx >= 0)
        { 
          for (n=0; n < num_members; n++)
            if (required_member_idx == (int) path_members[n])
              break;
          if (n == num_members)
            continue;
        }
      scan = new jx_path_filler;
      if (!scan->init(this,num_members,path_members,path_start))
        {
          delete scan;
          continue;
        }
      scan->next = head;
      head = scan;
    }

  // At this point, we see if any of the paths discovered so far completely
  // enclose other paths.  If so, the inner path is considered to provide
  // a hole in the interior of the outer path.  To make the concept more
  // definitive, each `jx_path_filler' object has a `container' pointer,
  // which is set to point to another `jx_path_filler' object which contains
  // it.  Paths which are contained may not contain other paths, since they
  // are considered holes in their containing path, so the containment
  // relationships between all paths must be established to set up these
  // pointers correctly.  At that point, we can move the boundary elements
  // from a contained `jx_path_filler' object into its container, reversing
  // the sense of the interior and exterior edges associated with each
  // path brick.  The contained path filler retains its own recollection of
  // the original path members which is was built from, so they can be
  // removed from the collection.
  for (scan=head; scan != NULL; scan=scan->next)
    { // First find the most immediate container for each path, if any.
      jx_path_filler *ct;
      for (ct=head; ct != NULL; ct=ct->next)
        if ((ct != scan) && ct->contains(scan))
          {
            if (scan->container == NULL)
              scan->container = ct;
            else if (scan->container->contains(ct))
              scan->container = ct;
          }
    }
  bool done=false;
  while (!done)
    { // Now progressively remove the "contained" status of paths which are
      // doubly contained.  This needs to be progressive so that if A contains
      // B which contains C which contains D which contains E which contains
      // F, we will be left with A, C and E as outer paths and B, D and F as
      // contained paths.
      for (done=true, scan=head; scan != NULL; scan=scan->next)
        if ((scan->container != NULL) &&
            (scan->container->container != NULL) &&
            (scan->container->container->container == NULL))
          { done = false; scan->container = NULL; }
    }
  for (scan=head; scan != NULL; scan=scan->next)
    {
      if (scan->container != NULL)
        continue;
      for (jx_path_filler *hole=head; hole != NULL; hole=hole->next)
        if (hole->container == scan)
          {
            scan->import_internal_boundary(hole);
            for (jx_path_filler *exc=hole->next; exc != NULL; exc=exc->next)
              if ((exc->container == scan) && exc->intersects(hole))
                exc->container = NULL; // Remove intersecting holes
          }
    }
  
  success = true;
  if (head == NULL)
    return kdu_dims(); // No closed paths
  
  // Cancel the selection and remove the regions we have moved to the
  // path filling objects.  To do this, we make use of the fact that each
  // `jx_path_filler' object keeps a collection of flag bits corresponding
  // to the original locations of all quadrilateral regions with which it
  // was initialized.
  kdu_dims result = cancel_selection();
  path_edge_flags_valid = shared_edge_flags_valid = false;
  bool state_pushed = false;
  for (n=0; n < 8; n++)
    path_flags[n] = 0;
  for (scan=head; scan != NULL; scan=scan->next)
    scan->get_original_path_members(path_flags);
  kdu_uint32 mask=1, *pflags=path_flags;
  for (n=0; n < num_regions; n++, mask<<=1)
    {
      if (mask == 0) { mask=1; pflags++; }
      if (!(*pflags & mask))
        continue; // This region is not being deleted
      if (!state_pushed)
        { 
          push_current_state();
          state_pushed = true;
        }
      for (int m=n+1; m < num_regions; m++)
        regions[m-1] = regions[m];
      num_regions--;
      n--; // So it stays the same on the next loop iteration
    }

  // Now we are ready to actually fill the paths one by one, adding the
  // resulting regions back in
  for (scan=head; scan != NULL; scan=scan->next)
    { 
      if (scan->container != NULL)
        continue; // This path filler provides a hole for its container.
      if (!scan->process())
        success = false;
      if ((num_regions+scan->num_regions) > 255)
        { success = false; break; }
      for (n=0; n < scan->num_regions; n++, num_regions++)
        { 
          jpx_roi *roi = regions+num_regions;
          kdu_coords *vertices = scan->region_vertices + 4*n;
          roi->init_quadrilateral(vertices[0],vertices[1],
                                  vertices[2],vertices[3]);
          roi->clip_region();
          update_extremities(roi);
        }
    }
  
  // Clean up all the path filling resources
  while ((scan=head) != NULL)
    { head=scan->next; delete scan; }
  
  kdu_dims bb; get_bounding_box(bb,false);
  result.augment(bb);
  return result;
}

/*****************************************************************************/
/*                    jpx_roi_editor::set_path_thickness                     */
/*****************************************************************************/

kdu_dims
  jpx_roi_editor::set_path_thickness(int thickness, bool &success)
{
  if (thickness < 1)
    thickness = 1;
  if (!path_edge_flags_valid)
    find_path_edge_flags();
  
  kdu_dims bb;
  get_bounding_box(bb,false);

  bool any_change = false;
  push_current_state();
  
  // Start by modifying any open ends of path bricks, along with ellipses
  // whose centres coincide with path segment end points.
  int p, n;
  success = true;
  for (n=0; n < num_regions; n++)
    { 
      jpx_roi *roi = regions + n;
      kdu_byte edges = path_edge_flags[n];
      kdu_byte type = edges >> 6;  edges&=0x0F;
      if (type == 0)
        continue;
      if ((type == 3) && roi->is_elliptical)
        { // Force ellipse to a circle of the required diameter
          kdu_coords extent, centre = roi->region.pos;
          centre.x += (roi->region.size.x>>1);
          centre.y += (roi->region.size.y>>1);
          extent.x = extent.y = (thickness-1)>>1;
          roi->init_ellipse(centre,extent,kdu_coords(),
                            roi->is_encoded,roi->coding_priority);
          any_change = true;
          continue;
        }
      assert((type == 1) || (type == 2));
      jpx_roi copy = *roi;
      bool modified_something = false;
      for (p=0; p < 4; p++)
        { 
          if (edges & (kdu_byte)(1<<p))
            continue; // Looking only for open ends
          if (((type == 1) && (p & 1)) || ((type == 2) && !(p & 1)))
            continue; // Not one of the edges defining the path segment
          kdu_coords p0, p1, ep0, ep1;
          p0 = jx_midpoint(roi->vertices[p],roi->vertices[(p+1)&3]);
          p1 = jx_midpoint(roi->vertices[(p+2)&3],roi->vertices[(p+3)&3]);
          double delta = 0.5*(thickness-1);
          do {
            if (!(jx_find_path_edge_intersection(&p1,&p0,NULL,-delta,&ep0) &&
                  jx_find_path_edge_intersection(&p1,&p0,NULL,delta,&ep1)))
              delta = -1.0; // Just a way of indicating a problem
            delta *= 1.1; // In case we need to iterate
          } while ((delta > 0.0) && (ep0 == ep1));
          if (delta < 0.0)
            { success = false; continue; }
          modified_something = true;
          kdu_coords adj = p0 - jx_midpoint(ep0,ep1);
          ep0 += adj;  ep1 += adj;
          copy.vertices[p] = ep0;  copy.vertices[(p+1)&3] = ep1;
          copy.flags |= JPX_QUADRILATERAL_ROI; // Ensure vertices count
        }
      if (modified_something)
        { 
          if (copy.check_geometry())
            { *roi = copy; any_change = true; }
          else
            success = false;
        }
    }
  
  // Finally, adjust connected ends of path bricks
  int m, q;
  for (n=0; n < num_regions; n++)
    { 
      jpx_roi *roi = regions + n;
      kdu_byte edges = path_edge_flags[n];
      kdu_byte type = edges >> 6;  edges&=0x0F;
      if ((type == 0) || (type == 3) || (edges == 0))
        continue;
      for (p=0; p < 4; p++)
        { 
          if (!(edges & (kdu_byte)(1<<p)))
            continue;
          kdu_coords v1=roi->vertices[p];
          kdu_coords v2=roi->vertices[(p+1)&3];
          for (m=0; m < num_regions; m++)
            {
              if ((m == n) || (path_edge_flags[m] == 0))
                continue;
              for (q=0; q < 4; q++)
                if ((path_edge_flags[m] & (kdu_byte)(1<<q)) &&
                    (((regions[m].vertices[q]==v2) &&
                      (regions[m].vertices[(q+1)&3]==v1)) ||
                     ((regions[m].vertices[q]==v1) &&
                      (regions[m].vertices[(q+1)&3]==v2))))
                  break;
              if (q < 4)
                break;
            }
          if (m == num_regions)
            continue; // Should not happen
          jpx_roi *nbr = regions+m;
          path_edge_flags[m] &= ~(1<<q); // Cancel edge flag in our neighbour
          path_edge_flags[n] &= ~(1<<p); // Cancel the edge flag here
          kdu_coords p0, p1, p2, ep0, ep1;
          p0 = jx_midpoint(roi->vertices[p],roi->vertices[(p+1)&3]);
          p1 = jx_midpoint(roi->vertices[(p+2)&3],roi->vertices[(p+3)&3]);
          p2 = jx_midpoint(nbr->vertices[(q+2)&3],nbr->vertices[(q+3)&3]);
          double delta = 0.5*(thickness-1);
          do {
            if (!(jx_find_path_edge_intersection(&p1,&p0,&p2,-delta,&ep0) &&
                  jx_find_path_edge_intersection(&p1,&p0,&p2,delta,&ep1)))
              delta = -1.0; // Just a way of indicating a problem
            delta *= 1.1; // In case we need to iterate
          } while ((delta > 0.0) && (ep0 == ep1));
          if (delta < 0.0)
            { success = false; continue; }
          jpx_roi roi_copy = *roi;
          jpx_roi nbr_copy = *nbr;
          kdu_coords adj =
            (jx_midpoint(roi->vertices[p],roi->vertices[(p+1)&3]) -
             jx_midpoint(ep0,ep1));
          ep0 += adj;  ep1 += adj;
          roi_copy.vertices[p] = ep0;
          roi_copy.vertices[(p+1)&3] = ep1;
          roi_copy.flags |= JPX_QUADRILATERAL_ROI; // Ensure vertices count
          nbr_copy.vertices[q] = ep1;
          nbr_copy.vertices[(q+1)&3] = ep0;
          nbr_copy.flags |= JPX_QUADRILATERAL_ROI; // Ensure vertices count
          if (roi_copy.check_geometry() && nbr_copy.check_geometry())
            { *roi = roi_copy; *nbr = nbr_copy; any_change = true; }
          else
            success = false;
        }
    }
  
  kdu_dims result;
  path_edge_flags_valid = shared_edge_flags_valid = false;
  if (any_change)
    {
      for (n=0; n < num_regions; n++)
        { // We need to do this, since we have been moving vertices directly;
          // we can't fix inconsistencies as we go, since this may rotate the
          // vertices and hence interfere with the interpretation of the
          // `path_edge_flags'.
          regions[n].fix_inconsistencies();
          regions[n].clip_region();
          update_extremities(regions+n);
        }
      result.augment(bb);
      get_bounding_box(bb,false);
      result.augment(bb);
    }
  else
    undo(); // Undo the effect of `push_current_state'
  return result;
}

/*****************************************************************************/
/*                   jpx_roi_editor::find_path_edge_flags                    */
/*****************************************************************************/

void jpx_roi_editor::find_path_edge_flags()
{
  if (path_edge_flags_valid)
    return;
  
  memset(path_edge_flags,0,(size_t) num_regions);
  int n, m, p, q;
  kdu_byte mask;
  for (n=0; n < num_regions; n++)
    { 
      jpx_roi *roi = regions + n;
      if (roi->is_elliptical)
        continue; // Elliptical regions do not contribute path segments
      kdu_byte edges=0; // For edges shared with other quadrilaterals
      kdu_byte multiple_edges=0; // For edges shared with multiple quads
      kdu_byte singular_edges=0; // For edges which are a single point
      kdu_byte elliptical_centres=0; // For edges shared with ellipses
      int ellipse_ids[4]={0,0,0,0}; // Id's of ellipses which are shared
      int num_real_edges=4;
      for (p=0; p < 4; p++)
        if (roi->vertices[p] == roi->vertices[(p+1)&3])
          num_real_edges--;
      if (num_real_edges == 0)
        continue; // Singularities cannot be path bricks
      assert(num_real_edges >= 2);
      if (num_real_edges == 3)
        continue; // Triangles cannot be path bricks
      for (m=0; m < num_regions; m++)
        { 
          if (m == n)
            continue;
          jpx_roi *nbr = regions + m;
          if (!nbr->region.intersects(roi->region))
            continue;
          for (mask=1, p=0; p < 4; p++, mask<<=1)
            { 
              kdu_coords v1=roi->vertices[p], v2=roi->vertices[(p+1)&3];
              if ((num_real_edges == 2) && (v1 != v2))
                continue; // Otherwise path segment would have length 0
              if (nbr->is_elliptical)
                { 
                  double cx=0.5*v1.x+0.5*v2.x, cy=0.5*v1.y+0.5*v2.y;
                  cx -= (nbr->region.pos.x+(nbr->region.size.x>>1));
                  cy -= (nbr->region.pos.y+(nbr->region.size.y>>1));
                  if ((-0.51 < cx) && (cx < 0.51) &&
                      (-0.51 < cy) && (cy < 0.51))
                    { 
                      elliptical_centres |= mask;
                      ellipse_ids[p] = m;
                    }
                  continue;
                }
              if (v1 == v2)
                singular_edges |= mask;
              for (q=0; q < 4; q++)
                { 
                  if ((v1 == nbr->vertices[q]) &&
                      ((v2 == nbr->vertices[(q-1)&3]) ||
                       ((v1 != v2) && (v2 == nbr->vertices[(q+1)&3]))))
                    { // Found a shared edge
                      if (edges & mask)
                        multiple_edges |= mask;
                      edges |= mask;
                    }
                }
            }
        }
      
      if (multiple_edges & ~(singular_edges|elliptical_centres))
        continue; // Don't allow multiply shared edges unless they are single
                  // points or elliptical centres
      
      edges |= elliptical_centres; // Now holds the "compatible edges"
      kdu_byte non_ellipse_compatible_edges =
        edges & (~elliptical_centres) & ~(singular_edges & multiple_edges);
        // These are the edges we are actually going to flag.  By not flagging
        // elliptical centres or singular edges which are multiply connected,
        // we ensure that these edges will be squared up and regenerated in
        // isolation by `set_path_thickness'.
      
      kdu_coords v1, v2; // Collect end-points of the path segment
      if (edges == 0)
        { // This region is a potential path brick defined by opposing edges
          kdu_long dx, dy, len_sq[2];
          for (p=0; p < 2; p++)
            {
              v1 = jx_midpoint(roi->vertices[p],roi->vertices[p+1]);
              v2 = jx_midpoint(roi->vertices[p+2],roi->vertices[(p+3)&3]);
              dx = v1.x-v2.x;  dy = v1.y-v2.y;
              len_sq[p] = dx*dx + dy*dy;
            }
          if (len_sq[0] >= len_sq[1])
            {
              path_edge_flags[n] = 0x40;
              edges = 1; // For the membership tests below
            }
          else
            {
              path_edge_flags[n] = 0x80;
              edges = 2; // For the membership tests below
            }
        }
      else if ((edges & 5) && !(edges & 10))
        { // This region is a potential path brick defined by edge 0 and edge 2
          path_edge_flags[n] = non_ellipse_compatible_edges | 0x40;
        }
      else if ((edges & 10) && !(edges & 5))
        { // This region is a potential path brick defined by edge 1 and edge 3
          path_edge_flags[n] = non_ellipse_compatible_edges | 0x80;
        }
      else
        continue; // Not a path brick
      
      // Now check whether the path segment is contained within the region.
      if (edges & 5)
        { p = 0; elliptical_centres &= 5; }
      else
        { p = 1; elliptical_centres &= 10; }
      v1 = jx_midpoint(roi->vertices[p],roi->vertices[p+1]);
      v2 = jx_midpoint(roi->vertices[p+2],roi->vertices[(p+3)&3]);
      if (roi->check_edge_intersection((p-1)&3,v1,v2) ||
          roi->check_edge_intersection((p+1)&3,v1,v2))
        { // Not a path brick
          path_edge_flags[n] = 0;
          continue;
        }
      
      // Finally, see about marking ellipses which coincide with us
      for (mask=1, p=0; p < 4; p++, mask<<=1)
        if (elliptical_centres & mask)
          { 
            m = ellipse_ids[p];
            assert((m >= 0) && (m < num_regions) && regions[m].is_elliptical);
            path_edge_flags[m] = 0xC0;
          }
    }

  path_edge_flags_valid = true;
}

/*****************************************************************************/
/*                  jpx_roi_editor::find_shared_edge_flags                   */
/*****************************************************************************/

void jpx_roi_editor::find_shared_edge_flags()
{
  if (shared_edge_flags_valid)
    return;
  memset(shared_edge_flags,0,(size_t) num_regions);
  int n, m, p, q;
  kdu_byte mask;
  for (n=0; n < num_regions; n++)
    { 
      jpx_roi *roi = regions + n;
      if (roi->is_elliptical)
        continue; // Elliptical regions do not share edges
      kdu_byte edges=0; // For edges shared with other quadrilaterals
      for (m=0; m < num_regions; m++)
        { 
          if (m == n)
            continue;
          jpx_roi *nbr = regions + m;
          if (nbr->is_elliptical || !nbr->region.intersects(roi->region))
            continue;
          for (mask=1, p=0; p < 4; p++, mask<<=1)
            if (!(edges & mask))
              { 
                kdu_coords v1=roi->vertices[p], v2=roi->vertices[(p+1)&3];
                for (q=0; q < 4; q++)
                  { 
                    if ((v1 == nbr->vertices[q]) &&
                        ((v2 == nbr->vertices[(q-1)&3]) ||
                         (v2 == nbr->vertices[(q+1)&3])))
                      edges |= mask;
                  }
              }
        }
      shared_edge_flags[n] = edges;
    }
  shared_edge_flags_valid = true;
}

/*****************************************************************************/
/*                      jpx_roi_editor::find_anchors                         */
/*****************************************************************************/

int
  jpx_roi_editor::find_anchors(kdu_coords anchors[], const jpx_roi &roi) const
{
  if (mode == JPX_EDITOR_VERTEX_MODE)
    { // Anchors are vertices or elliptical extremities
      for (int p=0; p < 4; p++)
        anchors[p] = roi.vertices[p];
      return 4;
    }
  else
    { // Anchors are edge mid-points or elliptical centre
      if (roi.is_elliptical)
        {
          anchors[0].x = roi.region.pos.x + (roi.region.size.x>>1);
          anchors[0].y = roi.region.pos.y + (roi.region.size.y>>1);
          return 1;
        }
      else
        {
          for (int p=0; p < 4; p++)
            anchors[p] = jx_midpoint(roi.vertices[p],roi.vertices[(p+1)&3]);
          return 4;
        }
    }
}

/*****************************************************************************/
/*                 jpx_roi_editor::set_drag_flags_for_vertex                 */
/*****************************************************************************/

void jpx_roi_editor::set_drag_flags_for_vertex(kdu_coords v)
{
  int n, p;
  for (n=0; n < num_regions; n++)
    {
      jpx_roi *roi = regions + n;
      if (roi->is_elliptical)
        { // See if `v' lies essentially on the boundary of the ellipse
          if (drag_flags[n] & 0x0F)
            continue; // Must be dragging/moving an ellipse anchor directly
          double x0=v.x, y0=v.y, xp, yp;
          if (roi->find_boundary_projection(x0,y0,xp,yp,0.98) < 0)
            continue;
          drag_flags[n] = 0x0F;
          set_drag_flags_for_boundary(roi);
        }
      else
        {
          kdu_byte mask;
          for (mask=1, p=0; p < 4; p++, mask<<=1)
            if (((drag_flags[n] & mask) == 0) && (roi->vertices[p] == v))
              drag_flags[n] |= mask;
        }
    }
}

/*****************************************************************************/
/*                jpx_roi_editor::set_drag_flags_for_midpoint                */
/*****************************************************************************/

void jpx_roi_editor::set_drag_flags_for_midpoint(kdu_coords point)
{
  int n, p;
  for (n=0; n < num_regions; n++)
    {
      jpx_roi *roi = regions + n;
      if (roi->is_elliptical)
        continue;
      kdu_coords anchors[4];
      int num_anchors = find_anchors(anchors,*roi);
      if ((num_anchors != 4) || (mode == JPX_EDITOR_VERTEX_MODE))
        return;
      for (p=0; p < 4; p++)
        if (anchors[p] == point)
          {
            kdu_byte mask = (kdu_byte)(1<<p);
            if (!(drag_flags[n] & mask))
              {
                drag_flags[n] |= mask;
                set_drag_flags_for_vertex(roi->vertices[p]);
              }
            mask = (kdu_byte)(1<<((p+1)&3));
            if (!(drag_flags[n] & mask))
              {
                drag_flags[n] |= mask;
                set_drag_flags_for_vertex(roi->vertices[p]);
              }
          }            
    }
}

/*****************************************************************************/
/*               jpx_roi_editor::set_drag_flags_for_boundary                 */
/*****************************************************************************/

void jpx_roi_editor::set_drag_flags_for_boundary(const jpx_roi *elliptical_roi)
{
  int n, p;
  for (n=0; n < num_regions; n++)
    {
      jpx_roi *roi = regions + n;
      if (roi->is_elliptical)
        continue;
      kdu_byte mask;
      for (mask=1, p=0; p < 4; p++, mask<<=1)
        if ((drag_flags[n] & mask) == 0)
          {
            double x0=roi->vertices[p].x, y0=roi->vertices[p].y, xp, yp;
            if (elliptical_roi->find_boundary_projection(x0,y0,xp,yp,
                                                         0.98) >= 0)
              drag_flags[n] |= mask;
          }
    }
}


/*****************************************************************************/
/*                     jpx_roi_editor::remove_duplicates                     */
/*****************************************************************************/

kdu_dims jpx_roi_editor::remove_duplicates()
{
  kdu_dims result;
  int n, m, p, q;
  for (n=0; n < num_regions; n++)
    { 
      jpx_roi *roi = regions + n;
      int duplicate_idx = -1;
      for (m=n+1; m < num_regions; m++)
        { 
          jpx_roi *roi2 = regions + m;
          if (roi2->is_elliptical != roi->is_elliptical)
            continue;
          if (*roi2 == *roi)
            { 
              duplicate_idx = (m==region_idx)?n:m;
              break;
            }
          if (!roi->is_elliptical)
            { // See if one region's edges are a subset of the other's
              for (p=0; p < 4; p++)
                { 
                  kdu_coords *v1=roi2->vertices+p;
                  kdu_coords *v2=roi2->vertices+((p+1)&3);
                  if (*v1 == *v2)
                    continue;
                  for (q=0; q < 4; q++)
                    { 
                      kdu_coords *w1=roi->vertices+q;
                      kdu_coords *w2=roi->vertices+((q+1)&3);
                      if (((*v1 == *w1) && (*v2 == *w2)) ||
                          ((*v1 == *w2) && (*v2 == *w1)))
                        break; // Edge p of `roi2' is matched in `roi'
                    }
                  if (q == 4)
                    break; // This edge was not matched
                }
              if (p == 4)
                { // All edges of `roi2' were matched in `roi'
                  duplicate_idx = m;
                  break;
                }
              
              for (p=0; p < 4; p++)
                { 
                  kdu_coords *v1=roi->vertices+p;
                  kdu_coords *v2=roi->vertices+((p+1)&3);
                  if (*v1 == *v2)
                    continue;
                  for (q=0; q < 4; q++)
                    { 
                      kdu_coords *w1=roi2->vertices+q;
                      kdu_coords *w2=roi2->vertices+((q+1)&3);
                      if (((*v1 == *w1) && (*v2 == *w2)) ||
                          ((*v1 == *w2) && (*v2 == *w1)))
                        break; // Edge p of `roi' is matched in `roi2'
                    }
                  if (q == 4)
                    break; // This edge was not matched
                }
              if (p == 4)
                { // All edges of `roi' were matched in `roi2'
                  duplicate_idx = n;
                  break;
                }
            }
        }
      if (duplicate_idx < 0)
        continue;
      path_edge_flags_valid = shared_edge_flags_valid = false;
      if (duplicate_idx == region_idx)
        result = cancel_selection();
      num_regions--;
      for (m=duplicate_idx; m < num_regions; m++)
        { 
          regions[m] = regions[m+1];
          drag_flags[m] = drag_flags[m+1];
        }
      if (duplicate_idx == n)
        n--; // So we correctly consider the next region. 
    }
  return result;  
}

/*****************************************************************************/
/*                   jpx_roi_editor::update_extremities                      */
/*****************************************************************************/

void
  jpx_roi_editor::update_extremities(jpx_roi *roi, kdu_coords *pt,
                                     int pt_idx)
{
  if (roi->is_elliptical)
    { 
      kdu_coords centre;
      double tan_theta, axis_extents[2];
      roi->get_ellipse(centre,axis_extents,tan_theta);
      if (pt != NULL)
        { // The region is perfectly circular.  In this case, the extremities
          // are not well-defined, so we can choose `tan_theta' such that
          // one of the extremities is aligned as closely as possible to `pt'.
          int dx = pt->x - centre.x;
          int dy = pt->y - centre.y;
          double tan_theta2 = 0.0;
          if ((dx != 0) && (dy != 0))
            {
              if (dy < 0)
                { dy = -dy; dx = -dx; }
              if ((dx >= -dy) && (dx <= dy))
                tan_theta2 = -((double) dx) / ((double) dy);
              else
                tan_theta2 = ((double) dy) / ((double) dx);
            }
          if (fabs(tan_theta-tan_theta2) > fabs(tan_theta+1.0/tan_theta2))
            tan_theta = -1.0/tan_theta2;
          else
            tan_theta = tan_theta2;
        }
      
      double x[4], y[4];
      double cos_theta = 1.0 / sqrt(1 + tan_theta*tan_theta);
      double sin_theta = tan_theta * cos_theta;
      x[0] = centre.x + sin_theta*axis_extents[0];
      x[2] = centre.x - sin_theta*axis_extents[0];
      y[0] = centre.y - cos_theta*axis_extents[0];
      y[2] = centre.y + cos_theta*axis_extents[0];
      x[1] = centre.x + cos_theta*axis_extents[1];
      x[3] = centre.x - cos_theta*axis_extents[1];
      y[1] = centre.y + sin_theta*axis_extents[1];
      y[3] = centre.y - sin_theta*axis_extents[1];
      
      int p;
      int rotate = 0;
      if ((pt != NULL) && (pt_idx >= 0) && (pt_idx <= 3))
        { 
          double min_dist_sq=0.0;
          for (p=0; p < 4; p++)
            { 
              double dx=x[p]-pt->x, dy=y[p]-pt->y;
              double dist_sq = dx*dx + dy*dy;
              if ((p == 0) || (dist_sq < min_dist_sq))
                { min_dist_sq = dist_sq; rotate = p-pt_idx; }
            }
        }
      for (p=0; p < 4; p++)
        { 
          roi->vertices[p].x = (int) floor(x[(p+rotate)&3]+0.5);
          roi->vertices[p].y = (int) floor(y[(p+rotate)&3]+0.5);
        }
    }
  else if (!(roi->flags & JPX_QUADRILATERAL_ROI))
    {
      roi->vertices[0] = roi->region.pos;
      roi->vertices[1].x = roi->region.pos.x + roi->region.size.x - 1;
      roi->vertices[1].y = roi->vertices[0].y;
      roi->vertices[2].x = roi->vertices[1].x;
      roi->vertices[2].y = roi->region.pos.y + roi->region.size.y - 1;
      roi->vertices[3].x = roi->region.pos.x;
      roi->vertices[3].y = roi->vertices[2].y;
    }
}

/*****************************************************************************/
/*                    jpx_roi_editor::get_edge_region                        */
/*****************************************************************************/

kdu_dims
  jpx_roi_editor::get_edge_region(const jpx_roi *roi, int edge) const
{
  kdu_coords v1, v2;
  if (roi->is_elliptical)
    { 
      if (edge == 0)
        return roi->region;
      else if (edge == 1)
        { v1=roi->vertices[3]; v2=roi->vertices[1]; }
      else if (edge == 2)
        { v1=roi->vertices[0]; v2=roi->vertices[2]; }
      else
        return kdu_dims();
    }
  else if ((edge < 0) || (edge >= 4))
    return kdu_dims();
  else
    { v1=roi->vertices[edge]; v2=roi->vertices[(edge+1)&3]; }
  kdu_dims result;
  if (v1.x < v2.x)
    { result.pos.x = v1.x; result.size.x = v2.x+1-v1.x; }
  else
    { result.pos.x = v2.x; result.size.x = v1.x+1-v2.x; }
  if (v1.y < v2.y)
    { result.pos.y = v1.y; result.size.y = v2.y+1-v1.y; }
  else
    { result.pos.y = v2.y; result.size.y = v1.y+1-v2.y; }
  return result;
}

/*****************************************************************************/
/*                    jpx_roi_editor::get_edge_vertices                      */
/*****************************************************************************/

void jpx_roi_editor::get_edge_vertices(const jpx_roi *roi, int edge,
                                       kdu_coords &from, kdu_coords &to) const
{
  kdu_coords centre;
  centre.x = roi->region.pos.x + (roi->region.size.x>>1);
  centre.y = roi->region.pos.y + (roi->region.size.y>>1);
  if (roi->is_elliptical)
    { 
      if (edge == 1)
        { from=roi->vertices[3]; to=roi->vertices[1]; }
      else if (edge == 2)
        { from=roi->vertices[0]; to=roi->vertices[2]; }
      else
        from = to = centre;
    }
  else if ((edge < 0) || (edge >= 4))
    from = to = centre;
  else
    { from=roi->vertices[edge]; to=roi->vertices[(edge+1)&3]; }
}

/*****************************************************************************/
/*                  jpx_roi_editor::find_next_anchor_edge                    */
/*****************************************************************************/

int jpx_roi_editor::find_next_anchor_edge()
{
  if ((anchor_idx < 0) || (region_idx < 0) || (region_idx >= num_regions))
    return -1; 
  int result=0;
  if (regions[region_idx].is_elliptical)
    {
      if (mode == JPX_EDITOR_VERTEX_MODE)
        { // There is only one edge, corresponding to the ellipse itself
          result = (edge_idx < 0)?0:-1;
        }
      else if (mode == JPX_EDITOR_SKELETON_MODE)
        { // Two selectable edges, corresponding to the horizontal and vertical
          // axes of the ellipse; these are assigned indices of 1 and 2,
          // respectively.
          if (edge_idx < 0)
            result = 1;
          else
            result = (edge_idx == 1)?2:-1;
          if ((result == 1) && (regions[region_idx].region.size.x <= 1))
            result = 2;
          if ((result == 2) && (regions[region_idx].region.size.y <= 1))
            result = -1;
          return result;
        }
      else
        return -1; // No selectable edges for ellipses in path mode
    }
  else
    { // Quadrilateral region is selected
      if (mode == JPX_EDITOR_VERTEX_MODE)
        { // There are up to two edges per anchor point
          if (edge_idx < 0)
            result = anchor_idx;
          else
            result = (edge_idx == anchor_idx)?((anchor_idx-1)&3):-1;
          if ((result == anchor_idx) &&
              (regions[region_idx].vertices[anchor_idx] ==
               regions[region_idx].vertices[(anchor_idx+1)&3]))
            result = (anchor_idx-1) & 3;
          if ((result == ((anchor_idx-1)&3)) &&
              (regions[region_idx].vertices[anchor_idx] ==
               regions[region_idx].vertices[(anchor_idx-1)&3]))
            result = -1;
        }
      else
        { // There is only one edge per anchor point
          result = (edge_idx < 0)?anchor_idx:-1;
          if (regions[region_idx].vertices[anchor_idx] ==
              regions[region_idx].vertices[(anchor_idx+1)&3])
            result = -1;
        }
    }
  return result;
}

/*****************************************************************************/
/*                      jpx_roi_editor::move_vertices                        */
/*****************************************************************************/

void
  jpx_roi_editor::move_vertices(jpx_roi *roi, kdu_byte dflags,
                                kdu_coords disp) const
{
  dflags &= 0x0F; // Just in case
  if (dflags == 0)
    return; // Nothing to move
  
  int last_moved_extremity = 0;
  int p;
  kdu_byte mask;
  kdu_coords v[4];
  for (p=0; p < 4; p++)
    v[p] = roi->vertices[p];
  for (mask=1, p=0; p < 4; p++, mask<<=1)
    if (dflags & mask)
      { v[p] += disp; last_moved_extremity = p; }
  if (roi->is_elliptical)
    {
      if (dflags == 0x0F)
        { // Move the entire ellipse -- easy
          roi->region.pos += disp;
          update_extremities(roi);
        }
      else
        { // Should be moving exactly one vertex
          assert((dflags==1) || (dflags==2) || (dflags==4) || (dflags==8));
          kdu_coords axis_vecs[2] =  {v[2]-v[0], v[1]-v[3]}; // vert then horz
          double x, y, axis_extents[2];
          for (p=0; p < 2; p++)
            { 
              if ((axis_vecs[p].x == 0) && (axis_vecs[p].y == 0))
                return; // Can't perform the move
              x = axis_vecs[p].x; y = axis_vecs[p].y;
              axis_extents[p] = 0.5*sqrt(x*x + y*y);
            }
          
          // Determine which axis is being dragged
          p = (last_moved_extremity & 1);
          kdu_coords centre = jx_midpoint(v[last_moved_extremity],
                                          v[(last_moved_extremity+2)&3]);
          if (axis_vecs[p].y < 0)
            { axis_vecs[p].x=-axis_vecs[p].x; axis_vecs[p].y=-axis_vecs[p].y; }
          double tan_theta = 0.0;
          int near_vertical_axis = 0;
          if (axis_vecs[p].y == 0)
            { // The axis being aligned is horizontal
              near_vertical_axis = 1-p;
              tan_theta = 0.0;
            }
          else
            { 
              tan_theta = -axis_vecs[p].x / (double) axis_vecs[p].y;
              if ((tan_theta < -1.0) || (tan_theta > 1.0))
                { tan_theta = -1.0 / tan_theta; near_vertical_axis = 1-p; }
              else
                near_vertical_axis = p;
            }
          if (near_vertical_axis != 0)
            { double tmp=axis_extents[0];
              axis_extents[0]=axis_extents[1]; axis_extents[1]=tmp; }
          roi->init_ellipse(centre,axis_extents,tan_theta,
                            roi->is_encoded,roi->coding_priority);
          update_extremities(roi,&v[last_moved_extremity],
                             last_moved_extremity);
        }
    }
  else
    { 
      roi->init_quadrilateral(v[0],v[1],v[2],v[3],roi->is_encoded,
                              roi->coding_priority);
      if (!roi->check_geometry())
        { // Try reversing vertices
          kdu_coords tmp=roi->vertices[1];
          roi->vertices[1]=roi->vertices[3];
          roi->vertices[3]=tmp;
        }
      update_extremities(roi);
    }
}

/*****************************************************************************/
/*                   jpx_roi_editor::delete_selected_region                  */
/*****************************************************************************/

kdu_dims jpx_roi_editor::delete_selected_region()
{
  kdu_dims result;
  if ((anchor_idx < 0) || (region_idx < 0) || (region_idx >= num_regions) ||
      (num_regions < 2))
    return result;
  
  push_current_state();
  
  int n = region_idx;
  result = cancel_selection();  
  result.augment(regions[n].region);
  num_regions--;
  for (; n < num_regions; n++)
    regions[n] = regions[n+1];
  path_edge_flags_valid = shared_edge_flags_valid = false;
  
  // Augment by new bounding box because path segments may have changed a lot
  kdu_dims bb;
  get_bounding_box(bb,false);
  result.augment(bb);
  
  return result;
}

/*****************************************************************************/
/*                    jpx_roi_editor::measure_complexity                     */
/*****************************************************************************/

double jpx_roi_editor::measure_complexity() const
{
  int n, element_count = 0;
  for (n=0; n < num_regions; n++)
    element_count += (regions[n].is_simple())?1:2;
  return (element_count==255)?1.0:(element_count*(1.0/255.0));
}

/*****************************************************************************/
/*                         jpx_roi_editor::add_region                        */
/*****************************************************************************/

kdu_dims jpx_roi_editor::add_region(bool ellipses, kdu_dims visible_frame)
{
  kdu_dims result = cancel_drag();
  if ((anchor_idx < 0) || (region_idx < 0) || (region_idx >= num_regions))
    return result;
  if (num_regions >= 255)
    return result; // No space for more regions
  if (measure_complexity() > 0.99)
    return result;
  
  kdu_coords new_selection_anchor_point = anchor_point;

  push_current_state();
  
  // Now add the new region
  jpx_roi *src_roi = regions+region_idx;
  jpx_roi new_roi;
  
  if (mode == JPX_EDITOR_PATH_MODE)
    { // In path mode, we always add at least a quadrilateral region.
      int n, p;
      bool have_ellipse=false; // True if there is an elliptical centre already
      kdu_coords centre, extent, skew, disp, ep0, ep1, ep2, ep3;
         // The created path segment will extent from `centre' to
         // `centre'+`disp'.  The edge which passes through `centre' will have
         // coordinates `ep0' and `ep1'.
      kdu_coords v[4];
      if (src_roi->get_ellipse(centre,extent,skew))
        { // Look for a quadrilateral
          have_ellipse = true;
          for (n=0; n < num_regions; n++)
            if (!regions[n].is_elliptical)
              { 
                find_anchors(v,regions[n]);
                for (p=0; p < 4; p++)
                  if (v[p] == centre)
                    break;
                if (p < 4)
                  break;
              }
          if (n < num_regions)
            { // Found one
              ep0 = regions[n].vertices[p];
              ep1 = regions[n].vertices[(p+1)&3];
              disp = anchor_point - v[(p+2)&3];
            }
          else
            { // Base the new quadrilateral upon the bounding box of the
              // ellipse
              ep0 = ep1 = centre;
              if (extent.x < extent.y)
                { ep0.x-=extent.x;  ep1.x+=extent.x;  disp.y = 2*extent.y; }
              else
                { ep0.y-=extent.y;  ep1.y+=extent.y;  disp.x = 2*extent.x; }
            }
        }
      else
        { // Look for an existing ellipse
          ep0 = src_roi->vertices[anchor_idx];
          ep1 = src_roi->vertices[(anchor_idx+1)&3];
          if (get_path_segment_for_region(region_idx,ep2,ep3) &&
              (ep2 != anchor_point) && (ep3 != anchor_point))
            { // Unusual case in which selected anchor is not a path end-point
              disp.x = ep1.y-ep0.y;  disp.y = ep0.x-ep1.x;
              disp.x = (disp.x>0)?((disp.x+7)>>2):(disp.x>>2);
              disp.y = (disp.y>0)?((disp.y+7)>>2):(disp.y>>2);
              ep2 = ep0 + disp;  ep3 = ep1 + disp;
              if (!visible_frame.is_empty())
                { 
                  visible_frame.clip_point(ep2); visible_frame.clip_point(ep3);
                  disp = jx_midpoint(ep2-ep1,ep3-ep0);
                }
              ep0 += disp;  ep1 += disp;
            }
          disp = anchor_point -
            jx_midpoint(src_roi->vertices[(anchor_idx+2)&3],
                        src_roi->vertices[(anchor_idx+3)&3]);
          for (n=0; n < num_regions; n++)
            if (regions[n].get_ellipse(centre,extent,skew) &&
                (centre == anchor_point))
              { have_ellipse = true; break; }
        }
      
      double dx=ep1.x-ep0.x, dy=ep1.y-ep0.y;
      double hlen = 0.5*sqrt(dx*dx+dy*dy);
      if ((num_regions < 254) && ellipses && !have_ellipse)
        { // Start by adding a circle
          centre = anchor_point;
          extent.x = extent.y = (int) floor(0.5+hlen);
          new_roi.init_ellipse(centre,extent,skew);
          new_roi.clip_region();
          if (new_roi.check_geometry())
            { 
              update_extremities(&new_roi);
              regions[num_regions++] = new_roi;
              have_ellipse = true;
              path_edge_flags_valid = shared_edge_flags_valid = false;
            }
        }
      
      // Now add the quadrilateral region
      ep2=ep1+disp; ep3=ep0+disp;
      if ((disp != kdu_coords()) && !visible_frame.is_empty())
        {
          visible_frame.clip_point(ep2); visible_frame.clip_point(ep3);
          disp = jx_midpoint(ep2-ep1,ep3-ep0);
          ep2 = ep1+disp;  ep3=ep0+disp;
        }
      new_roi.init_quadrilateral(ep0,ep1,ep2,ep3);
      if (!new_roi.check_geometry())
        { // Reverse ep0 and ep1
          ep2=ep0; ep0=ep1; ep1=ep2; ep2=ep1+disp; ep3=ep0+disp; 
        }
      if (have_ellipse)
        { 
          kdu_coords mp0=jx_midpoint(ep0,ep1), mp1=mp0+disp;
          do {
            if (!(jx_find_path_edge_intersection(&mp0,&mp1,NULL,-hlen,&ep2) &&
                  jx_find_path_edge_intersection(&mp0,&mp1,NULL,hlen,&ep3)))
              hlen = -1.0;
            hlen *= 1.1; // In case we need to iterate
          } while ((hlen > 0.0) && (ep2 == ep3));
          ep0 = ep3-disp;  ep1 = ep2-disp;
        }
      new_roi.init_quadrilateral(ep0,ep1,ep2,ep3);
      new_selection_anchor_point = jx_midpoint(ep2,ep3);
    }
  else if (ellipses)
    { 
      kdu_coords centre, extent, skew;
      if (src_roi->is_elliptical)
        { // Reproduce the existing ellipse at a different location
          src_roi->get_ellipse(centre,extent,skew);
          centre = src_roi->vertices[anchor_idx];
        }
      else
        { 
          kdu_coords from, to;
          get_edge_vertices(src_roi,edge_idx,from,to);
          centre.x = (from.x+to.x)>>1;
          centre.y = (from.y+to.y)>>1;
          double dx=from.x-to.x, dy=from.y-to.y;
          int len = (int)(0.5 + 0.5*sqrt(dx*dx+dy*dy));
          if (len < 5) len = 5;
          extent.x = extent.y = len;
        }
      new_roi.init_ellipse(centre,extent,skew);
    }
  else
    {
      if (regions[region_idx].is_elliptical && (edge_idx == 0))
        { // Make the new region equal to the ellipse's bounding box
          new_roi.init_rectangle(src_roi->region);
        }
      else if ((!src_roi->is_elliptical) && (mode != JPX_EDITOR_VERTEX_MODE))
        { // Add a quadrangle by mirroring the line joining the selected
          // anchor point to its opposite anchor point.
          kdu_coords v[4];
          find_anchors(v,*src_roi);
          kdu_coords from=src_roi->vertices[anchor_idx];
          kdu_coords to=src_roi->vertices[(anchor_idx+1)&3];
          kdu_coords vec = v[anchor_idx] - v[(anchor_idx+2) & 3];
          v[0] = to;
          v[1] = from;
          v[2] = from + vec;
          v[3] = to + vec;
          if (!visible_frame.is_empty())
            { 
              visible_frame.clip_point(v[2]);  visible_frame.clip_point(v[3]);
              vec = jx_midpoint(v[2]-from,v[3]-to);
              v[2] = from + vec; v[3] = to + vec;
            }
          new_roi.init_quadrilateral(v[0],v[1],v[2],v[3]);
        }
      else if ((!src_roi->is_elliptical) &&
               (((src_roi->vertices[anchor_idx] ==
                  src_roi->vertices[(anchor_idx+1) & 3]) &&
                 (src_roi->vertices[(anchor_idx+2)&3] ==
                  src_roi->vertices[(anchor_idx+3)&3])) ||
                ((src_roi->vertices[anchor_idx] ==
                  src_roi->vertices[(anchor_idx-1) & 3]) &&
                 (src_roi->vertices[(anchor_idx+2)&3] ==
                  src_roi->vertices[(anchor_idx+1)&3]))))
        { // Add a line by mirroring the line joining the anchor point
          // to its opposite point.
          kdu_coords v[2];
          v[0] = src_roi->vertices[anchor_idx];
          v[1] = src_roi->vertices[(anchor_idx+2)&3];
          kdu_coords vec = v[0] - v[1];  v[1] = v[0] + vec;
          if (!visible_frame.is_empty())
            visible_frame.clip_point(v[1]);
          new_roi.init_quadrilateral(v[0],v[0],v[1],v[1]);
        }
      else
        { 
          kdu_coords from, to;
          get_edge_vertices(src_roi,edge_idx,from,to);
          if ((edge_idx < 0) || (from == to))
            { // No active edge.; add a 5x5 square centred at the anchor point
              kdu_dims rect;
              rect.pos = anchor_point - kdu_coords(2,2);
              rect.size = kdu_coords(5,5);
              new_roi.init_rectangle(rect);
            }
          else
            { // Add a square which shares an edge with the active one
              kdu_coords v[4];
              v[0] = to;
              v[1] = from;
              kdu_coords vec; vec.x = -(from.y-to.y); vec.y = (from.x-to.x);
              v[2] = from + vec;
              v[3] = to + vec;
              new_roi.init_quadrilateral(v[0],v[1],v[2],v[3]);
            }
        }
    }
      
  new_roi.clip_region();
  if (!new_roi.check_geometry())
    return result;
  update_extremities(&new_roi);
  regions[num_regions++] = new_roi;
  path_edge_flags_valid = shared_edge_flags_valid = false;
  
  // Augment by new bounding box because path segments may have changed a lot
  kdu_dims bb;
  get_bounding_box(bb,false);
  result.augment(bb);
  
  if (new_selection_anchor_point != anchor_point)
    select_anchor(new_selection_anchor_point,false);
  remove_duplicates();

  return result;
}


/* ========================================================================= */
/*                          jx_scribble_converter                            */
/* ========================================================================= */

/*****************************************************************************/
/*                        jx_scribble_converter::init                        */
/*****************************************************************************/

void jx_scribble_converter::init(const kdu_coords *points, int num_points,
                                 bool want_fill)
{
  assert(num_points <= JX_ROI_SCRIBBLE_POINTS);
  if ((num_points > 1) && (points[0] == points[num_points-1]))
    num_points--;
  memcpy(scribble_points,points,sizeof(kdu_coords)*((size_t)num_points));
  this->num_scribble_points = num_points;
  this->num_boundary_vertices = 0;
  int n;
  jx_scribble_segment *seg;
  free_segments = segments = NULL;
  for (n=511; n >= 0; n--)
    { 
      seg = seg_store + n;
      seg->next = free_segments;
      free_segments = seg;
      seg->scribble_points = scribble_points;
      seg->num_scribble_points = num_scribble_points;
    }
  segments = seg = get_free_seg();
  seg->set_associated_points(0,num_scribble_points);
  if (want_fill)
    seg->prev = seg->next = seg; // Cyclically connected list of segments
}

/*****************************************************************************/
/*              jx_scribble_converter::find_boundary_vertices                */
/*****************************************************************************/

bool jx_scribble_converter::find_boundary_vertices()
{
  if (segments == NULL)
    return false;
  int n = 0;
  jx_scribble_segment *seg = segments;
  boundary_vertices[n++] = seg->seg_start;
  do {
    if ((n == 512) || !(seg->is_line || seg->is_ellipse))
      return false;
    boundary_vertices[n++] = seg->seg_end;
    seg = seg->next;
  } while ((seg != NULL) && (seg != segments));
  num_boundary_vertices = n;
  return true;
}

/*****************************************************************************/
/*                jx_scribble_converter::find_polygon_edges                  */
/*****************************************************************************/

bool jx_scribble_converter::find_polygon_edges(kdu_long max_sq_dist)
{
  assert(segments != NULL);
  jx_scribble_segment *seg = segments;
  do {
    if (!(seg->is_line || seg->is_ellipse))
      { // This segment holds an uncommitted set of scribble points.  We need
        // to build a piecewise linear approximation for it.
        if (seg->num_seg_points <= 0)
          return false; // Should not happen
        if ((seg->prev != NULL) &&
            (seg->prev->is_line || seg->prev->is_ellipse))
          seg->seg_start = seg->prev->seg_end;
        else
          seg->seg_start = *(seg->get_point(0));
        kdu_coords seg_end, v, w;
        int num_confirmed_seg_points=0;
        int n, p;
        for (n=0; n < seg->num_seg_points; n++)
          { // Try ending the segment at the n'th scribble point associated
            // with `seg'.
            if ((n == (seg->num_seg_points-1)) && (seg->next != NULL) &&
                (seg->next != seg))
              seg_end = seg->next->seg_start; // Must join to next segment
            else
              seg_end = *(seg->get_point(n));
            for (p=0; p <= n; p++) // Need to check point `n' as well
              { 
                v = w = *(seg->get_point(p));
                jx_project_to_line(seg->seg_start,seg_end,w);
                kdu_long dx=v.x-w.x, dy=v.y-w.y;
                if ((dx*dx+dy*dy) > max_sq_dist)
                  break;
              }
            if (p <= n)
              break; // Unable to extend segment end point
            
            if (n > 0)
              { // Next, check for intersections with other segments or
                // scribble points which have not yet been approximated.
                jx_scribble_segment *scan=segments;
            
                for (; scan != NULL;
                     scan=(scan->next==segments)?NULL:scan->next)
                  { 
                    const kdu_coords *from, *to;
                    if (scan->is_ellipse)
                      { 
                        continue; // Later, we will need to check this too
                      }
                    else if (scan->is_line)
                      { 
                        assert(scan != seg);
                        from = &(scan->seg_start); to = &(scan->seg_end);
                        if (jx_check_line_intersection(seg->seg_start,seg_end,
                                                       true,true,*from,*to))
                          break;
                      }
                    else
                      { // Check for intersections with scribble points
                        p = (scan==seg)?(n+1):0; // Idx of first point to check
                        if ((p == 0) && (scan->prev != NULL))
                          from = &(scan->prev->seg_end);
                        else if (p == 0)
                          { from = scan->get_point(p); p++; }
                        else
                          from = scan->get_point(p-1);
                        for (; (to = scan->get_point(p)) != NULL; p++, from=to)
                          if (jx_check_line_intersection(seg->seg_start,
                                                         seg_end,true,true,
                                                         *from,*to))
                            break;
                        if (to != NULL)
                          break;
                        if ((scan->next != NULL) &&
                            jx_check_line_intersection(seg->seg_start,seg_end,
                                                       true,true,*from,
                                                       scan->next->seg_start))
                          break;
                      }
                  }
                if (scan != NULL)
                  break; // Intersection detected
              }
            
            // If we get here, we have been able to extend the segment to
            // include the n'th point currently associated with `seg'.
            num_confirmed_seg_points = n+1;
            seg->seg_end = seg_end;
          }
 
        if (seg->next == seg)
          { // We are operating on the first and only segment in a cyclic
            // path.  In cyclic paths, disjoint association is accomplished
            // by enforcing the rule that the first segment is not associated
            // with the first scribble point.  It was to begin with, so that
            // we could get a line segment started, but now we want to remove
            // the association with the first point.  We do this my making
            // some minor adjustments so as to ensure that the following
            // statements automatically create the new segment.
            if (num_confirmed_seg_points < 2)
              return false; // Should not be possible; cyclic path with 1 point
            seg->first_seg_point++;
            num_confirmed_seg_points--;
          }
        if (num_confirmed_seg_points < seg->num_seg_points)
          { // Need to create at least one more segment
            jx_scribble_segment *new_seg = get_free_seg();
            if (new_seg == NULL)
              return false; // Exceeded available segment resources
            new_seg->set_associated_points(seg->first_seg_point +
                                           num_confirmed_seg_points,
                                           seg->num_seg_points -
                                           num_confirmed_seg_points);
            seg->num_seg_points = num_confirmed_seg_points;
            new_seg->prev = seg;
            new_seg->next = seg->next;
            seg->next = new_seg;
            if (seg->prev == seg)
              seg->prev = new_seg;
          }
        seg->is_line = true;
      }
    seg = seg->next;
  } while ((seg != NULL) && (seg != segments));
  return true;
}
  

/* ========================================================================= */
/*                              jx_path_filler                               */
/* ========================================================================= */

/*****************************************************************************/
/*                     jx_path_filler::init (first form)                     */
/*****************************************************************************/

bool
  jx_path_filler::init(jpx_roi_editor *editor, int num_path_members,
                       kdu_byte *path_members, kdu_coords path_start)
{
  if (num_path_members > JXPF_MAX_REGIONS)
    return false;
  
  // Start by filling out the `original_path_members' flags and the
  // `tmp_path_vertices' array, so we can determine where the external
  // boundaries are.  At the same time, we will move the quadrilateral
  // vertices into the `region_vertices' array.
  if (num_path_members > 256)
    return false; // Otherwise we exceed storage in `tmp_path_vertices' array
  next = NULL;
  container = NULL;
  num_tmp_path_vertices = num_regions = 0;
  memset(original_path_members,0,sizeof(kdu_uint32)*8);
  tmp_path_vertices[num_tmp_path_vertices++] = path_start;
  
  int n, p;
  for (n=0; n < num_path_members; n++)
    { 
      kdu_coords ep1, ep2;
      int idx = path_members[n];
      if (!editor->get_path_segment_for_region(idx,ep1,ep2))
        continue; // Should not happen
      const jpx_roi *roi = editor->get_region(idx);
      if ((roi == NULL) || roi->is_elliptical)
        continue;
      kdu_uint32 mask=(kdu_uint32)(1<<(idx & 31));
      original_path_members[idx>>5] |= mask;
      if (ep1 == ep2)
        continue;
      for (p=0; p < 4; p++)
        region_vertices[4*num_regions+p]=roi->vertices[p];
      num_regions++;
      path_start = (ep1==path_start)?ep2:ep1;
      tmp_path_vertices[num_tmp_path_vertices++] = path_start;
    }
  
  int sense = examine_path(tmp_path_vertices, num_tmp_path_vertices);
  if (sense == 0)
    return false; // Not an acceptable path

  // Rotate the vertices of the quadrilaterals so that they agree with the
  // direction of the path and also determine which direction the interior
  // of the path lies in.
  for (n=0; n < num_regions; n++)
    { 
      kdu_coords *vertices=region_vertices+4*n; // Points to 4 vertices
      kdu_coords *path_eps=tmp_path_vertices+n; // Points to 2 path end-points

      // First rotate the vertices so that path_eps[0] is the mid-point of
      // vertices 0 and 1 and path_eps[1] is the mid-point of vertices 2 and 3.
      int p0=-1;
      kdu_coords mp[4];
      for (p=0; p < 4; p++)
        { 
          mp[p] = jx_midpoint(vertices[p],vertices[(p+1)&3]);
          if (mp[p] == path_eps[0])
            p0 = p;
        }
      if ((p0 < 0) || (mp[(p0+2)&3] != path_eps[1]))
        return false;
      for (; p0 > 0; p0--)
        { // Rotate until `p0' = 0
          mp[0] = vertices[0];
          vertices[0] = vertices[1];
          vertices[1] = vertices[2];
          vertices[2] = vertices[3];
          vertices[3] = mp[0];
        }
    }
  
  // Now use the `sense' of the path to figure out where the
  // external edges and the shared edges are; we also adjust the interior
  // vertices of path bricks which cross only at their mid-points so that
  // all boundary points of the polygonal boundary to be filled correspond
  // to region vertices.
  kdu_coords prev_edge[2]; // Vertices at the end of the previous region
  kdu_coords first_edge[2]; // Vertices at the start of the first region
  prev_edge[0] = region_vertices[4*num_regions-2]; // Previous vertex 2
  prev_edge[1] = region_vertices[4*num_regions-1]; // Previous vertex 3
  first_edge[0] = region_vertices[0]; // First vertex 0
  first_edge[1] = region_vertices[1]; // First vertex 1
  int prev_n = num_regions-1;
  for (n=0; n < num_regions; prev_n=n, n++)
    {
      kdu_coords *vertices = region_vertices+4*n; // Points to four vertices
      kdu_coords *next_edge = (n<(num_regions-1))?(vertices+4):first_edge;
             // `next_edge' gives the first two vertices of the next edge
      int next_n = n+1;
      next_n = (next_n==num_regions)?0:next_n;
      kdu_coords *path_eps=tmp_path_vertices+n; // Points to 2 path end-points
      
      if (sense > 0)
        { // Outer edge runs from vertex 1 to vertex 2 (this is edge 1)
          region_edges[4*n+1] = -1; // Edge 1 is external
          region_edges[4*n+3] = JXPF_INTERNAL_EDGE; // Edge 3 internal unshared
          if (vertices[1] != prev_edge[0])
            { // Move vertex 0 to the path segment end-point
              vertices[0] = path_eps[0];
              region_edges[4*n+0] = -1; // Edge 0 is external
            }
          else if (vertices[0] != prev_edge[1])
            region_edges[4*n+0]=JXPF_INTERNAL_EDGE; // Edge 0 internal unshared
          else
            region_edges[4*n+0] = 4*prev_n+2; // Edge 0 shared with prev edge 2
          prev_edge[0] = vertices[2]; // Prepare for the next iteration
          prev_edge[1] = vertices[3];
          if (vertices[2] != next_edge[1])
            { // Move vertex 3 to the path segment end-point and fix it
              vertices[3] = path_eps[1];
              region_edges[4*n+2] = -1; // Edge 2 is external
            }
          else if (vertices[3] != next_edge[0])
            region_edges[4*n+2]=JXPF_INTERNAL_EDGE; // Edge 2 internal unshared
          else
            region_edges[4*n+2] = 4*next_n+0; // Edge 2 shared with next edge 0
        }
      else
        { // Outer edge runs from vertex 3 to vertex 0 (this is edge 3)
          region_edges[4*n+3] = -1; // Edge 3 is external
          region_edges[4*n+1] = JXPF_INTERNAL_EDGE; // Edge 1 internal unshared
          if (vertices[0] != prev_edge[1])
            { // Move vertex 1 to the path segment end-point and fix it
              vertices[1] = path_eps[0];
              region_edges[4*n+0] = -1; // Edge 0 is external
            }
          else if (vertices[1] != prev_edge[0])
            region_edges[4*n+0]=JXPF_INTERNAL_EDGE; // Edge 0 internal unshared
          else
            region_edges[4*n+0] = 4*prev_n+2; // Edge 0 shared with prev edge 2
          prev_edge[0] = vertices[2]; // Prepare for the next iteration
          prev_edge[1] = vertices[3];
          if (vertices[3] != next_edge[0])
            { // Move vertex 2 to the path segment end-point and fix it
              vertices[2] = path_eps[1];
              region_edges[4*n+2] = -1; // Edge 2 is external
            }
          else if (vertices[2] != next_edge[1])
            region_edges[4*n+2]=JXPF_INTERNAL_EDGE; // Edge 2 internal unshared
          else
            region_edges[4*n+2] = 4*next_n+0; // Edge 2 shared with next edge 0
        }
    }
  return true;
}

/*****************************************************************************/
/*                    jx_path_filler::init (second form)                     */
/*****************************************************************************/

bool
  jx_path_filler::init(const kdu_coords *src_vertices, int num_src_vertices)
{
  // Start by resetting various members
  next = NULL;
  container = NULL;
  num_tmp_path_vertices = num_regions = 0;
  memset(original_path_members,0,sizeof(kdu_uint32)*8);

  // See if the vertices form an allowable polygonal boundary and find the
  // sense of the boundary.
  if ((num_src_vertices < 3) || (num_src_vertices > JXPF_MAX_REGIONS))
    return false;
  int sense = examine_path(src_vertices,num_src_vertices);
  if (sense == 0)
    return false;
  
  // Now set up one initial quadrilateral for each edge of the polygon. We
  // could use less regions than this, of course, but starting with more
  // regions (and hence more free edges) allows the path filling algorithm
  // to find better filling strategies -- otherwise, we tend to get very
  // skewed regions which cost more to paint.
  int n;
  for (n=0; n < (num_src_vertices-1); n++, num_regions++)
    {
      assert(num_regions < JXPF_MAX_REGIONS);
      kdu_coords *vertices = region_vertices + 4*num_regions;
      int *edges = region_edges + 4*num_regions;
      vertices[0] = vertices[3] = src_vertices[n];
      vertices[1] = vertices[2] = src_vertices[n+1];
      if (sense > 0)
        { // Vertices run clockwise around boundary
          edges[0] = -1;  edges[2] = JXPF_INTERNAL_EDGE;
        }
      else
        { // Vertices run anti-clockwise around boundary
          edges[2] = -1; edges[0] = JXPF_INTERNAL_EDGE;
        }
      int next_region_idx = (n<(num_src_vertices-2))?(n+1):0;
      int prev_region_idx = (n==0)?(num_src_vertices-2):(n-1);
      edges[1] = 4*next_region_idx+3;
      edges[3] = 4*prev_region_idx+1;
    }
  if (!check_integrity())
    { assert(0); return false; }
  return true;
}

/*****************************************************************************/
/*                       jx_path_filler::examine_path                        */
/*****************************************************************************/

int jx_path_filler::examine_path(const kdu_coords *vertices, int num_vertices)
{
  // Make sure the vertices form a closed loop
  if ((num_vertices < 3) || (vertices[0] != vertices[num_vertices-1]))
    return 0;
  
  // Next, check whether or not the path intersects itself.  Intersections
  // at path segment end-points are also problematic, so long as we check
  // only path segment end-points which are not adjacent to one another.
  int n, p;
  for (n=2; n < (num_vertices-1); n++)
    {
      kdu_coords ep1 = vertices[n], ep2 = vertices[n+1];
      p = (n==(num_vertices-2))?1:0;
      for (; p < (n-1); p++)
        {
          kdu_coords pA = vertices[p], pB = vertices[p+1];
          if ((pA == ep1) || (pA == ep2) || (pB == ep1) || (pB == ep2))
            return 0; // Non-adjacent segs shouldn't have common end-points
          // Need to know if (ep1->ep2) crosses (pA->pB).  The code here is
          // essentially reproduced from `jpx_roi::check_edge_intersection'.
          kdu_long AmB_x=pA.x-pB.x, AmB_y=pA.y-pB.y;
          kdu_long DmC_x=ep2.x-ep1.x, DmC_y=ep2.y-ep1.y;
          kdu_long AmC_x=pA.x-ep1.x, AmC_y=pA.y-ep1.y;
          kdu_long det = AmB_x * DmC_y - DmC_x * AmB_y;
          kdu_long t = DmC_y*AmC_x - DmC_x * AmC_y;
          kdu_long u = AmB_x*AmC_y - AmB_y * AmC_x;
          if (det < 0) { det=-det; t=-t; u=-u; }
          if ((t > 0) && (t < det) && (u > 0) && (u < det))
            return 0; // The path segments cross each other
        }
    }
  
  // Work out the direction of the boundary.  There are many ways to determine
  // the direction (or sense) of a boundary.  In the following, we do this by
  // projecting a vector in the rightward and leftward direction from the
  // centre of each boundary segment and determining whether or not it
  // intersects any other boundary segment.  If there is no intersection
  // in some direction, that path segment indicates the sense of the boundary,
  // "almost certainly".  The only caveat to this is that an intersection
  // might be missed if the path segment's mid-point is rounded to one of
  // its end-points and that end-point happens to be shared with an
  // adjacent segment where an intersection would be found with the true
  // mid-point.  We could track down this special case by considering two
  // different rounding policies for the mid-point, but we prefer simply to
  // count the number of missing intersections on the left and the number
  // of missing intersections on the right and take the majority vote.
  int sense = 0;
  for (n=0; n < (num_vertices-1); n++)
    {
      const kdu_coords *path_eps = vertices+n; // Points to 2 path end-points
      kdu_coords rvec; // Rightward pointing vector as we follow path segment
      rvec.x = path_eps[0].y-path_eps[1].y;
      rvec.y = path_eps[1].x-path_eps[0].x;
      kdu_coords pA = jx_midpoint(path_eps[0],path_eps[1]);
      bool path_lies_to_left=false, path_lies_to_right=false;
      for (p=n+1; (p=(p<(num_vertices-1))?p:0) != n; p++)
        { // Consider all the other segments
          // Need to know if (pA -> pA+/-rvec) crosses
          // [pC->pD] where pC and pD are the end-points of path segment p.
          // That is, we want to solve pA+t*rvec = pC+u*(pD-pC) for u in
          // the closed interval [0,1]. If such a u exists, we are
          // interested in the sign of t.  That is, we have
          // pC-pA = t*rvec+u*(pC-pD).
          kdu_coords pC = vertices[p], pD = vertices[p+1];
          kdu_long CmD_x=pC.x-pD.x, CmD_y=pC.y-pD.y;
          kdu_long CmA_x=pC.x-pA.x, CmA_y=pC.y-pA.y;
          kdu_long det = rvec.x*CmD_y - rvec.y*CmD_x;
          kdu_long t_by_det = CmD_y*CmA_x - CmD_x*CmA_y;
          kdu_long u_by_det = rvec.x*CmA_y - rvec.y*CmA_x;
          if (det < 0)
            { det=-det; t_by_det=-t_by_det; u_by_det=-u_by_det; }
          if ((u_by_det < 0) || (u_by_det > det))
            continue; // u does not belong to [0,1).
          if (t_by_det < 0)
            path_lies_to_left = true;
          else if (t_by_det > 0)
            path_lies_to_right = true;
        }
      if (path_lies_to_left != path_lies_to_right)
        { // This segment readily identifies the sense of the path
          if (path_lies_to_right)
            { // We must be traversing the path clockwise
              sense++;
            }
          else
            { // We must be traversing the path anti-clockwise
              sense--;
            }
        }
    }
  return sense;
}

/*****************************************************************************/
/*                  jx_path_filler::import_internal_boundary                 */
/*****************************************************************************/

void jx_path_filler::import_internal_boundary(const jx_path_filler *src)
{
  if (((src->num_regions + this->num_regions) > JXPF_MAX_REGIONS) ||
      !contains(src))
    return;
  int n, offset=4*this->num_regions;
  this->num_regions += src->num_regions;
  for (n=0; n < (4*src->num_regions); n++)
    { 
      this->region_vertices[offset+n] = src->region_vertices[n];
      int idx = src->region_edges[n];
      if (idx < 0)
        idx = JXPF_INTERNAL_EDGE;
      else if (idx >= JXPF_INTERNAL_EDGE)
        idx = -1;
      else
        idx += offset;
      this->region_edges[offset+n] = idx;
    }
}

/*****************************************************************************/
/*                         jx_path_filler::contains                          */
/*****************************************************************************/

bool jx_path_filler::contains(const jx_path_filler *src) const
{
  if (intersects(src))
    return false;
  
  int n, p, src_edge_idx=0;
  const kdu_coords *src_vertices = src->region_vertices;
  for (n=0; n < src->num_regions; n++, src_vertices+=4)
    for (p=0; p < 4; p++, src_edge_idx++)
      {
        if (src->region_edges[src_edge_idx] >= 0)
          continue; // Not an external edge
        const kdu_coords *pA = src_vertices+p, *pB=src_vertices+((p+1)&3);
        kdu_coords src_rvec; // Right pointing vector as we follow the src edge
        src_rvec.x = pA->y-pB->y;  src_rvec.y = pB->x-pA->x;
        // We need to know if (pA -> pA+t*src_rvec) crosses [pC->pD] for any
        // negative value of t, where [pC->pD] is some external edge of the
        // current object.  If not, the `src' object is not completely
        // contained within us.  More to the point, we need to know that the
        // smallest value of |t| for which such an intersection occurs
        // corresponds to intersection from the inside of the current object.
        // The intersection occurs from the inside if the inner product between
        // `rvec' and a rightward pointing vector from the intersecting
        // external edge of the current object is positive.
        int m, q, edge_idx=0;
        const kdu_coords *vertices=this->region_vertices;
        double min_abs_t=-1.0;
        kdu_long ip_for_min_abs_t=-1; // Inner prod at min |t| should be +ve
        for (m=0; m < num_regions; m++, vertices+=4)
          { 
            for (q=0; q < 4; q++, edge_idx++)
              { 
                if (region_edges[edge_idx] >= 0)
                  continue; // Not an external edge
                const kdu_coords *pC=vertices+q, *pD=vertices+((q+1)&3);
                kdu_long CmD_x=pC->x-pD->x, CmD_y=pC->y-pD->y;
                kdu_long CmA_x=pC->x-pA->x, CmA_y=pC->y-pA->y;
                kdu_long det = src_rvec.x*CmD_y - src_rvec.y*CmD_x;
                kdu_long t_by_det = CmD_y*CmA_x - CmD_x*CmA_y;
                kdu_long u_by_det = src_rvec.x*CmA_y - src_rvec.y*CmA_x;
                if (det < 0)
                  { det=-det; t_by_det=-t_by_det; u_by_det=-u_by_det; }
                if ((u_by_det >= 0) && (u_by_det <= det) &&
                    (det > 0) && (t_by_det < 0))
                  { // We have an intersection
                    double abs_t = -((double) t_by_det) / ((double) det);
                    if ((min_abs_t > 0.0) && (abs_t > min_abs_t))
                      continue;
                    min_abs_t = abs_t;
                    kdu_coords rvec;
                    rvec.x = pC->y-pD->y;  rvec.y = pD->x-pC->x;
                    ip_for_min_abs_t =(((kdu_long) rvec.x)*src_rvec.x +
                                       ((kdu_long) rvec.y)*src_rvec.y);
                  }                
              }
          }
        if (ip_for_min_abs_t < 0)
          return false; // Not contained
      }
  return true;
}

/*****************************************************************************/
/*                        jx_path_filler::intersects                         */
/*****************************************************************************/

bool jx_path_filler::intersects(const jx_path_filler *src) const
{
  int n, p;
  const kdu_coords *vertices=region_vertices;
  for (n=0; n < num_regions; n++, vertices+=4)
    for (p=0; p < 4; p++)
      {
        const kdu_coords *v1=vertices+p, *v2=vertices+((p+1)&3);
        int m, q;
        const kdu_coords *src_vertices=src->region_vertices;
        for (m=0; m < src->num_regions; m++, src_vertices+=4)
          for (q=0; q < 4; q++)
            {
              const kdu_coords *w1=src_vertices+q, *w2=src_vertices+((q+1)&3);
                 // Need to know if (w1->w2) crosses (v1->v2).  The code is
                 // essentially that of `jpx_roi::check_edge_intersection'.
              kdu_long AmB_x=v1->x-v2->x, AmB_y=v1->y-v2->y;
              kdu_long DmC_x=w2->x-w1->x, DmC_y=w2->y-w1->y;
              kdu_long AmC_x=v1->x-w1->x, AmC_y=v1->y-w1->y;
              kdu_long det = AmB_x*DmC_y - DmC_x*AmB_y;
              kdu_long t_by_det = DmC_y*AmC_x - DmC_x * AmC_y;
              kdu_long u_by_det = AmB_x*AmC_y - AmB_y * AmC_x;
              if (det < 0)
                { det=-det; t_by_det=-t_by_det; u_by_det=-u_by_det; }
              if ((t_by_det >= 0) && (t_by_det <= det) && (det > 0) &&
                  (u_by_det >= 0) && (u_by_det <= det))
                return true;
            }
      }
  return false;    
}

/*****************************************************************************/
/*                 jx_path_filler::check_boundary_violation                  */
/*****************************************************************************/

bool jx_path_filler::check_boundary_violation(const kdu_coords *C,
                                              const kdu_coords *D) const
{
  int n, p, edge_idx=0;
  const kdu_coords *vertices = region_vertices;
  for (n=0; n < num_regions; n++, vertices+=4)
    for (p=0; p < 4; p++, edge_idx++)
      {
        if (region_edges[edge_idx] >= 0)
          continue; // Not an external edge
        kdu_coords pA = vertices[p], pB = vertices[(p+1)&3];
        // Need to know if (v0->v1) crosses (pA->pB).  The code here is
        // essentially reproduced from `jpx_roi::check_edge_intersection'.
        kdu_long AmB_x=pA.x-pB.x, AmB_y=pA.y-pB.y;
        kdu_long DmC_x=D->x-C->x, DmC_y=D->y-C->y;
        kdu_long AmC_x=pA.x-C->x, AmC_y=pA.y-C->y;
        kdu_long det = AmB_x * DmC_y - DmC_x * AmB_y;
        kdu_long t = DmC_y*AmC_x - DmC_x * AmC_y;
        kdu_long u = AmB_x*AmC_y - AmB_y * AmC_x;
        if (det < 0) { det=-det; t=-t; u=-u; }
        if ((t > 0) && (t < det) && (u > 0) && (u < det))
          return true; // The line segments cross each other
      }
  return false;
}

/*****************************************************************************/
/*                 jx_path_filler::check_boundary_violation                  */
/*****************************************************************************/

bool jx_path_filler::check_boundary_violation(const jpx_roi &roi) const
{
  int n, p, q, edge_idx=0;
  kdu_coords v[4];
  roi.get_quadrilateral(v[0],v[1],v[2],v[3]);
  const kdu_coords *vertices = region_vertices;
  for (n=0; n < num_regions; n++, vertices+=4)
    for (p=0; p < 4; p++, edge_idx++)
      { 
        if (region_edges[edge_idx] >= 0)
          continue; // Not an external edge
        kdu_coords pA = vertices[p], pB = vertices[(p+1)&3];
        int u_type[4] = {0,0,0,0};
           // The above two arrays are used to record information discovered
           // regarding intersections between each edge and the infinitely
           // extended line which passes through pA and pB.  Specifically, for
           // the q'th edge, running from v1_q=v[q] to v2_q=v[(q+1)&3], let
           // x be the intersection point such that x=v1_q+t(v2_q-v1_q) and
           // x=pA+u(pB-pA).  We consider an intersection to exist if t > 0
           // and t <= 1 -- that is, we count intersections only with the first
           // of the two end-points of edge q.  We record u_type[q] = 0 if
           // there is no intersection, u_type[q] = 1 if the intersection
           // corresponds to u <= 0, u_type[q] = 3 if the intersection
           // corresponds to u >= 1 and u_type[q] = 2 if the intersection
           // corresponds to 0 < u < 1.  For a boundary violation, we need
           // two distinct values of q, such that u_type[q1] and u_type[q2]
           // are both non-zero and have different values.
        for (q=0; q < 4; q++)
          { // Fill out the u_type values.  To find the values of u and t,
            // note that (v1_q-v2_q  pB-pA)*(t u)^t = v1_q-pA.
            kdu_long V1m2_x = v[q].x-v[(q+1)&3].x;
            kdu_long V1m2_y = v[q].y-v[(q+1)&3].y;
            kdu_long BmA_x = pB.x-pA.x, BmA_y = pB.y-pA.y;
            kdu_long V1mA_x = v[q].x-pA.x, V1mA_y = v[q].y-pA.y;
            kdu_long det = V1m2_x*BmA_y - V1m2_y*BmA_x;
            kdu_long t = BmA_y*V1mA_x - BmA_x*V1mA_y;
            kdu_long u = V1m2_x*V1mA_y - V1m2_y*V1mA_x;
            if (det == 0)
              continue; // Lines are parallel; intersections are not a problem
            if (det < 0)
              { det = -det; t = -t; u = -u; }
            if ((t <= 0) || (t > det))
              continue; // No intersection with (v1_q,v2_q].
            if (u <= 0)
              u_type[q] = 1;
            else if (u >= det)
              u_type[q] = 3;
            else
              u_type[q] = 2;
            
            // Now see if we have another intersection -- for violation
            if ((q == 0) || (u_type[q] == 0))
              continue;
            int ref_q;
            for (ref_q=0; ref_q < q; ref_q++)
              if ((u_type[ref_q] != 0) && (u_type[ref_q] != u_type[q]))
                return true; // Found boundary violation
          }
      }
  return false;
}
  
/*****************************************************************************/
/*                          jx_path_filler::process                          */
/*****************************************************************************/

bool jx_path_filler::process()
{
  bool early_finish = false;
  while (join()) {
    if (early_finish) return true;
  }
  while (simplify()) {
    if (early_finish) return true;
  }
  int num_internal_edges = count_internal_edges();
  if (num_internal_edges > 0)
    { // Simplification may have split apart a zero length edge, rendering it
      // internal, so some extra joining may be in order.
      while (join()) {
        if (early_finish) return true;
      }
      num_internal_edges = count_internal_edges();
    }
  
  if (num_internal_edges > 0)
    { 
      if (early_finish)
        return true;
      while (fill_interior()) {
        if (early_finish)
          return true;
        join();
      }
      if (early_finish)
        return true;
      while (simplify());
      while (join());
      num_internal_edges = count_internal_edges();
    }
  if (num_internal_edges == 1)
    assert(0); // This never makes sense
  return (num_internal_edges == 0);
}

/*****************************************************************************/
/*                    jx_path_filler::check_integrity                        */
/*****************************************************************************/

bool jx_path_filler::check_integrity()
{
  bool result = true;
  int n, m, num_edges=num_regions<<2;
  for (n=0; n < num_edges; n++)
    if (((m=region_edges[n]) < 0) || (m == JXPF_INTERNAL_EDGE))
      continue;
    else if ((m > JXPF_MAX_VERTICES) || (region_edges[m] != n)) 
      {
        result = false;
        break;
      }
  return result;
}

/*****************************************************************************/
/*                  jx_path_filler::count_internal_edges                     */
/*****************************************************************************/

int jx_path_filler::count_internal_edges()
{
  int n, p, edge_idx, count=0;
  kdu_coords *vertices=region_vertices;
  for (n=edge_idx=0; n < num_regions; n++, vertices+=4)
    for (p=0; p < 4; p++, edge_idx++)
      {
        assert(region_edges[edge_idx] <= JXPF_INTERNAL_EDGE);
        if ((region_edges[edge_idx] == JXPF_INTERNAL_EDGE) &&
            (vertices[p] != vertices[(p+1)&3]))
          count++;
      }
  return count;
}

/*****************************************************************************/
/*                           jx_path_filler::join                            */
/*****************************************************************************/

bool jx_path_filler::join()
{
  bool any_change = false;
  int n, p, edge_idx=0;
  kdu_coords *vertices = region_vertices;
  for (n=0; n < num_regions; n++, vertices+=4)
    for (p=0; p < 4; p++, edge_idx++)
      { 
        assert(region_edges[edge_idx] <= JXPF_INTERNAL_EDGE);
        if (region_edges[edge_idx] != JXPF_INTERNAL_EDGE)
          continue; // This edge is already either external or shared
        const kdu_coords *ep[2] = // End-points of the `edge_idx' edge
          { vertices+p, vertices+((p+1)&3) };
        
        int best_join_idx = -1; // Best edge to join to
        bool best_one_vertex_exact = false;
        kdu_long best_dist_sq=0; // For finding best edge
        const kdu_coords *best_ep[2]; // Best substitutes for *ep[i]
        kdu_coords *join_vertices = region_vertices;
        int m, q, join_idx=0; // Stands for candidate edge to "join" to
        for (m=0; m < num_regions; m++, join_vertices+=4)
          for (q=0; q < 4; q++, join_idx++)
            {
              if (m == n)
                continue; // Only join to another region
              assert(region_edges[join_idx] <= JXPF_INTERNAL_EDGE);
              if (region_edges[join_idx] != JXPF_INTERNAL_EDGE)
                continue; // This edge is not available for sharing
              kdu_coords *jp[2] = // Reversed end-points of the `join_idx' edge
                { join_vertices+((q+1)&3), join_vertices+q};
              if (!check_vertex_changes_for_edge(edge_idx,jp[0],jp[1]))
                continue;
              kdu_long dx, dy, dist_sq=0;
              bool one_vertex_exact=false;
              for (int i=0; i < 2; i++)
                {
                  dx = ep[i]->x - jp[i]->x; dy = ep[i]->y - jp[i]->y;
                  if ((dx|dy) == 0)
                    one_vertex_exact = true;
                  dist_sq += dx*dx + dy*dy;
                }
              if ((best_join_idx < 0) ||
                  (one_vertex_exact && !best_one_vertex_exact) ||
                  ((one_vertex_exact == best_one_vertex_exact) &&
                   (dist_sq < best_dist_sq)))
                { best_join_idx=join_idx; best_dist_sq = dist_sq;
                  best_one_vertex_exact = one_vertex_exact;
                  best_ep[0]=jp[0]; best_ep[1]=jp[1]; }
            }

        if (best_join_idx >= 0)
          { // Perform the join
            any_change = true;
            apply_vertex_changes_for_edge(edge_idx,best_ep[0],best_ep[1]);
            region_edges[edge_idx] = best_join_idx;
            region_edges[best_join_idx] = edge_idx;
          }
      }
  
  return any_change;
}

/*****************************************************************************/
/*                         jx_path_filler::simplify                          */
/*****************************************************************************/

bool jx_path_filler::simplify()
{
  bool any_change = false;
  int n, p, edge_idx;
  
  kdu_coords *vertices = region_vertices;
  for (n=0; n < num_regions; n++, vertices+=4)
    { 
      if (remove_degenerate_region(n))
        { 
          n--; vertices-=4; // So loop correctly visits the next region
          continue;
        }
      
      // Let's see if two adjacent triangles can be converted to a single
      // quadrilateral.
      int tri_edges1[3], tri_edges2[3];
      if (is_region_triangular(n,tri_edges1))
        { 
          int nbr_idx=-1;
          for (p=0; p < 3; p++)
            {
              nbr_idx = region_edges[tri_edges1[p]];
              if ((nbr_idx >= 0) && (nbr_idx != JXPF_INTERNAL_EDGE) &&
                  is_region_triangular(nbr_idx>>2,tri_edges2))
                break;
            }
          if (p < 3)
            { // Found two adjacent triangles
              int k = nbr_idx>>2; // k is the index of the second region
              int te1_next = tri_edges1[(p==2)?0:(p+1)]; // Non-shared edges
              int te1_prev = tri_edges1[(p==0)?2:(p-1)]; // from triangle 1
              for (p=0; p < 3; p++)
                if (tri_edges2[p] == nbr_idx)
                  break; // Found the common edge in the second triangle
              assert(p < 3);
              int te2_next = tri_edges2[(p==2)?0:(p+1)]; // Non-shared edges
              int te2_prev = tri_edges2[(p==0)?2:(p-1)]; // from triangle 2
              int quad_edges[4] = {te1_next,te1_prev,te2_next,te2_prev};
                    // Clockwise ordered edges for the combined quadrilateral
              int nbr_edges[4];
              kdu_coords quad_vertices[4];
              for (p=0; p < 4; p++)
                { // Find the connectivity of the existing edges we are
                  // replacing.
                  quad_vertices[p] = region_vertices[quad_edges[p]];
                  nbr_edges[p] = region_edges[quad_edges[p]];
                  assert((nbr_edges[p] < 0) ||
                         (nbr_edges[p] == JXPF_INTERNAL_EDGE) ||
                         (region_edges[nbr_edges[p]] == quad_edges[p]));
                  nbr_idx = nbr_edges[p]>>2;
                  if ((nbr_idx == n) || (nbr_idx == k))
                    break; // More than one of the triangular edges in common
                }
              if (p == 4)
                { // Now we are committed to reducing the triangles to a quad
                  remove_edge_references_to_region(k); // So can reuse region k
                  int k_by_4 = k<<2;
                  for (p=0; p < 4; p++)
                    { // Write the new quadrilateral into region k
                      region_vertices[k_by_4+p] = quad_vertices[p];
                      region_edges[k_by_4+p] = nbr_edges[p];
                      if ((nbr_edges[p] >= 0) &&
                          (nbr_edges[p] != JXPF_INTERNAL_EDGE))
                        region_edges[nbr_edges[p]] = k_by_4+p;
                    }
                  remove_region(n);
                  n--; vertices-=4; // So loop correctly visits the next region
                  continue; // Go back to the main loop
                }
            }
        }
      
      // Let's try and make the region degenerate by squashing one vertex
      // to its the opposite one.
      for (edge_idx=4*n, p=0; p < 4; p++, edge_idx++)
        { // Consider moving vertices[p]          
          if (vertices[p] == vertices[(p-1)&3])
            continue; // We don't want to split up already joined vertices
          const kdu_coords *ep[2]={vertices+((p+2)&3),vertices+((p+1)&3)};
             // These are the new end-points we are considering for the edge
          if (vertices[p] == *ep[1])
            { // The region is triangular
              if (!check_vertex_changes_for_edge(edge_idx,ep[0],ep[0]))
                ep[0] = vertices+((p-1)&3); // Collapse the other way
              ep[1] = ep[0]; // Joined vertices move together
            }
          if (!check_vertex_changes_for_edge(edge_idx,ep[0],ep[1]))
            continue; // The move is not possible
          
          // If we get here, we can collapse region n
          apply_vertex_changes_for_edge(edge_idx,ep[0],ep[1]);
          break;
        }
      if ((p < 4) && remove_degenerate_region(n))
        { 
          any_change = true;
          n--; vertices-=4; // So loop correctly visits the next region
        }
    }
  return any_change;
}

/*****************************************************************************/
/*                       jx_path_filler::fill_interior                       */
/*****************************************************************************/

bool jx_path_filler::fill_interior()
{
  int n, p, edge_idx;
  
  kdu_coords *vertices = region_vertices;
  for (n=edge_idx=0; n < num_regions; n++, vertices+=4)
    for (p=0; p < 4; p++, edge_idx++)
      {
        if ((region_edges[edge_idx] != JXPF_INTERNAL_EDGE) ||
            (vertices[p] == vertices[(p+1)&3]))
          continue; // Not an interior edge
        // Now look for other internal edges which share the same end-points
        int m, q, alt_edge_idx;
        int nbr_edge_idx[2]={-1,-1};
        kdu_coords *alt_vertices=region_vertices;
        for (m=alt_edge_idx=0; m < num_regions; m++, alt_vertices+=4)
          for (q=0; q < 4; q++, alt_edge_idx++)
            {
              if ((alt_edge_idx == edge_idx) ||
                  (region_edges[alt_edge_idx] != JXPF_INTERNAL_EDGE) ||
                  (alt_vertices[q] == alt_vertices[(q+1)&3]))
                continue;
              if (alt_vertices[q] == vertices[(p+1)&3])
                nbr_edge_idx[0] = alt_edge_idx;
              if (alt_vertices[(q+1)&3] == vertices[p])
                nbr_edge_idx[1] = alt_edge_idx;
            }
        if ((nbr_edge_idx[0] >= 0) && (nbr_edge_idx[1] >= 0) &&
            add_quadrilateral(nbr_edge_idx[0],edge_idx,nbr_edge_idx[1]))
          return true;
        else if ((nbr_edge_idx[0] >= 0) &&
                 add_triangle(nbr_edge_idx[0],edge_idx))
          return true;
        else if ((nbr_edge_idx[1] >= 0) &&
                 add_triangle(edge_idx,nbr_edge_idx[1]))
          return true;
      }
  return false;
}

/*****************************************************************************/
/*              jx_path_filler::check_vertex_changes_for_edge                */
/*****************************************************************************/

bool jx_path_filler::check_vertex_changes_for_edge(int edge_idx,
                                                   const kdu_coords *v0,
                                                   const kdu_coords *v1,
                                                   int initial_edge_idx) const
{
  if (edge_idx == initial_edge_idx)
    return true;
  int n=edge_idx>>2, p=edge_idx&3;
  const kdu_coords *vertices = region_vertices + (n<<2);
  bool is_changed[2] = {vertices[p]!=*v0, vertices[(p+1)&3]!=*v1};
  if (!(is_changed[0] || is_changed[1]))
    return true;
  if (region_edges[edge_idx] < 0)
    return false; // Cannot move the edge, because it is external
  jpx_roi test_roi;
  test_roi.init_quadrilateral(vertices[(p-1)&3],*v0,*v1,vertices[(p+2)&3]);
  if (!test_roi.check_geometry())
    return false; // Not a valid quadrilateral
  if (check_boundary_violation(test_roi))
    return false;
  
  if (initial_edge_idx < 0)
    { // Need to explore the effect on any neighbour of `edge_idx' itself
      initial_edge_idx = edge_idx;
      if ((region_edges[edge_idx] < 0) ||
          check_boundary_violation(v0,v1) ||
          ((region_edges[edge_idx] != JXPF_INTERNAL_EDGE) &&
           !check_vertex_changes_for_edge(region_edges[edge_idx],v1,v0,
                                          initial_edge_idx)))
        return false;
    }

  // Now explore the edge and its neighbours to see if the move is allowed.
  int test_edge_idx = (n<<2) + ((p-1)&3); // Neighbouring edge to the left
  if (is_changed[0] && (test_edge_idx != initial_edge_idx))
    { 
      const kdu_coords *ep[2] = {vertices+((p-1)&3),v0};
      if ((region_edges[test_edge_idx] < 0) ||
          check_boundary_violation(ep[0],ep[1]) ||
          ((region_edges[test_edge_idx] != JXPF_INTERNAL_EDGE) &&
           !check_vertex_changes_for_edge(region_edges[test_edge_idx],
                                          ep[1],ep[0],initial_edge_idx)))
        return false;
    }
  test_edge_idx = (n<<2) + ((p+1)&3); // Neighbouring edge to the right
  if (is_changed[1] && (test_edge_idx != initial_edge_idx))
    { 
      const kdu_coords *ep[2] = {v1,vertices+((p+2)&3)};
      if ((region_edges[test_edge_idx] < 0) ||
          check_boundary_violation(ep[0],ep[1]) ||
          ((region_edges[test_edge_idx] != JXPF_INTERNAL_EDGE) &&
           !check_vertex_changes_for_edge(region_edges[test_edge_idx],
                                          ep[1],ep[0],initial_edge_idx)))
        return false;
    }
  
  // If we get here, all the tests have succeeded
  return true;
}

/*****************************************************************************/
/*              jx_path_filler::apply_vertex_changes_for_edge                */
/*****************************************************************************/

void jx_path_filler::apply_vertex_changes_for_edge(int edge_idx,
                                                   const kdu_coords *v0,
                                                   const kdu_coords *v1)
{
  assert((edge_idx >= 0) && (edge_idx < (num_regions*4)));
  int n=edge_idx>>2, p=edge_idx&3;
  kdu_coords *vertices = region_vertices+(n<<2);
  bool is_changed[2] = {vertices[p]!=*v0, vertices[(p+1)&3]!=*v1};
  if (!(is_changed[0] || is_changed[1]))
    return;
  assert(region_edges[edge_idx] >= 0);
  vertices[p] = *v0; vertices[(p+1)&3] = *v1;

  assert(region_edges[edge_idx] <= JXPF_INTERNAL_EDGE);
  if (region_edges[edge_idx] != JXPF_INTERNAL_EDGE)
    apply_vertex_changes_for_edge(region_edges[edge_idx],v1,v0);
  
  // Now propagate the change to neighbouring regions with which we share
  // an edge.
  int test_edge_idx = edge_idx + ((p-1)&3)-p; // Neighbouring edge to the left
  if (is_changed[0])
    { 
      const kdu_coords *ep[2] = {vertices+((p-1)&3), v0};
      assert(region_edges[test_edge_idx] <= JXPF_INTERNAL_EDGE);
      if (region_edges[test_edge_idx] != JXPF_INTERNAL_EDGE)
        apply_vertex_changes_for_edge(region_edges[test_edge_idx],ep[1],ep[0]);
    }
  test_edge_idx = (n<<2) + ((p+1)&3); // Neighbouring edge to the right
  if (is_changed[1])
    { 
      const kdu_coords *ep[2] = {v1, vertices+((p+2)&3)};
      assert(region_edges[test_edge_idx] <= JXPF_INTERNAL_EDGE);
      if (region_edges[test_edge_idx] != JXPF_INTERNAL_EDGE)
        apply_vertex_changes_for_edge(region_edges[test_edge_idx],ep[1],ep[0]);
    }
}

/*****************************************************************************/
/*                  jx_path_filler::is_region_triangular                     */
/*****************************************************************************/

bool jx_path_filler::is_region_triangular(int idx, int edges[])
{
  if ((idx < 0) || (idx == JXPF_INTERNAL_EDGE))
    return false;
  int k, p, base_n=4*idx;
  for (p=k=0; p < 4; p++)
    if (region_vertices[base_n+p] != region_vertices[base_n+((p+1)&3)])
      {
        if (k == 3) return false;
        edges[k++] = base_n+p;
      }
  return (k == 3);
}

/*****************************************************************************/
/*            jx_path_filler::remove_edge_references_to_region               */
/*****************************************************************************/

void jx_path_filler::remove_edge_references_to_region(int idx)
{
  int n, num_edges=num_regions<<2, base_idx=idx<<2;
  for (n=0; n < num_edges; n++)
    if ((region_edges[n] >= base_idx) && (region_edges[n] < (base_idx+4)))
      region_edges[n] = JXPF_INTERNAL_EDGE;
}

/*****************************************************************************/
/*                 jx_path_filler::remove_degenerate_region                  */
/*****************************************************************************/

bool jx_path_filler::remove_degenerate_region(int idx)
{
  if ((idx < 0) || (idx >= num_regions))
    return false;
  int base_idx = idx<<2;
  kdu_coords *vertices = region_vertices + base_idx;
  int e1[2], e2[2]; // Edges e1[i] and e2[i] are on top of each other
  if (vertices[0] == vertices[2])
    { // Edges 0->1 and 1->2 are overlapping, as are edges 2->3 and 3->0
      e1[0] = base_idx+0; e2[0] = base_idx+1;
      e1[1] = base_idx+2; e2[1] = base_idx+3;
    }
  else if (vertices[1] == vertices[3])
    { // Edges 1->2 and 2->3 are overlapping, as are edges 3->0 and 0->1
      e1[0] = base_idx+1; e2[0] = base_idx+2;
      e1[1] = base_idx+3; e2[1] = base_idx+0;
    }
  else if ((vertices[0]==vertices[1]) && (vertices[2]==vertices[3]))
    { // Edges 1->2 and 3->0 are overlapping
      e1[0] = base_idx+1; e2[0] = base_idx+3;
      e1[1] = e2[1] = -1;
    }
  else if ((vertices[1]==vertices[2]) && (vertices[3]==vertices[0]))
    { // Edges 2->3 and 0->1 are overlapping
      e1[0] = base_idx+2; e2[0] = base_idx+0;
      e1[1] = e2[1] = -1;
    }
  else
    return false;
  
  int p;
  for (p=0; p < 2; p++)
    { 
      if (e1[p] < 0)
        continue;
      if (region_edges[e1[p]] < 0)
        { int tmp=e1[p]; e1[p]=e2[p]; e2[p]=tmp; }
      if (region_edges[e1[p]] < 0)
        return false; // Both sides of the degenerate region are external
      if ((region_edges[e2[p]] < 0) &&
          (region_edges[e1[p]] == JXPF_INTERNAL_EDGE))
        return false; // Cannot transfer external edge to another region
    }
  
  // Now we are committed to removing the region; first exchange edge
  // connectivity
  for (p=0; p < 2; p++)
    { 
      if (e1[p] < 0)
        continue;
      int nbr1=region_edges[e1[p]], nbr2=region_edges[e2[p]];
      assert(nbr1 >= 0);
      if (nbr1 != JXPF_INTERNAL_EDGE)
        region_edges[nbr1] = nbr2;
      if ((nbr2 >= 0) && (nbr2 != JXPF_INTERNAL_EDGE))
        region_edges[nbr2] = nbr1;
      region_edges[e1[p]]=region_edges[e2[p]]=JXPF_INTERNAL_EDGE; // Not in use
    }
  for (p=0; p < 4; p++)
    { // Look for zero length edges which still think they are shared
      int nbr = region_edges[base_idx+p];
      if (nbr == JXPF_INTERNAL_EDGE) continue;
      region_edges[base_idx+p] = JXPF_INTERNAL_EDGE;
      if (nbr >= 0)
        region_edges[nbr] = JXPF_INTERNAL_EDGE;
    }
  
  // Now at last we can eliminate the region
  remove_region(idx);
  return true;
}

/*****************************************************************************/
/*                     jx_path_filler::remove_region                         */
/*****************************************************************************/

void jx_path_filler::remove_region(int idx)
{
  if ((idx < 0) || (idx >= JXPF_MAX_VERTICES))
    return;
  int base_idx = idx<<2;
  int n, p, *edges=region_edges;
  kdu_coords *vertices=region_vertices;
  for (n=0; n < idx; n++, edges+=4, vertices+=4)
    for (p=0; p < 4; p++)
      if ((edges[p] != JXPF_INTERNAL_EDGE) && (edges[p] >= base_idx))
        { // Have to adjust the edge reference
          if (edges[p] < (base_idx+4))
            { 
              assert(vertices[p] == vertices[(p+1)&3]);
              edges[p] = JXPF_INTERNAL_EDGE;
            }
          else
            edges[p] -= 4;
        }
  num_regions--;
  for (; n < num_regions; n++, edges+=4, vertices+=4)
    for (p=0; p < 4; p++)
      { 
        vertices[p] = vertices[p+4];
        if (((edges[p] = edges[p+4]) != JXPF_INTERNAL_EDGE) &&
            (edges[p] >= base_idx))
          { 
            if (edges[p] < (base_idx+4))
              { 
                assert(vertices[4+p] == vertices[4+((p+1)&3)]);
                edges[p] = JXPF_INTERNAL_EDGE;
              }
            else
              edges[p] -= 4;
          }
      }
}

/*****************************************************************************/
/*                    jx_path_filler::add_quadrilateral                      */
/*****************************************************************************/

bool jx_path_filler::add_quadrilateral(int shared_edge1, int shared_edge2,
                                       int shared_edge3)
{
  if (num_regions >= JXPF_MAX_REGIONS)
    return false;
  int shared_edges[3] = {shared_edge1,shared_edge2,shared_edge3};
  int p, shared_ends[3]; // Indices of the second vertex associated with edge
  for (p=0; p < 3; p++)
    shared_ends[p] = (shared_edges[p] & ~3) + (((shared_edges[p] & 3)+1)&3);
  assert(region_vertices[shared_edges[0]]==region_vertices[shared_ends[1]]);
  assert(region_vertices[shared_edges[1]]==region_vertices[shared_ends[2]]);
  kdu_coords vertices[4] =
    { region_vertices[shared_ends[0]], region_vertices[shared_ends[1]],
      region_vertices[shared_ends[2]], region_vertices[shared_edges[2]] };
  jpx_roi test_roi;
  test_roi.init_quadrilateral(vertices[0],vertices[1],vertices[2],vertices[3]);
  if (!test_roi.check_geometry())
    return false;
  if (check_boundary_violation(test_roi))
    return false;
  int n = num_regions++;
  int base_idx = n<<2;
  for (p=0; p < 4; p++)
    region_vertices[base_idx+p] = vertices[p];
  for (p=0; p < 3; p++)
    { 
      region_edges[base_idx+p] = shared_edges[p];
      region_edges[shared_edges[p]] = base_idx+p;
    }
  region_edges[base_idx+3] = JXPF_INTERNAL_EDGE;
  return true;
}

/*****************************************************************************/
/*                      jx_path_filler::add_triangle                         */
/*****************************************************************************/

bool jx_path_filler::add_triangle(int shared_edge1, int shared_edge2)
{
  if (num_regions >= JXPF_MAX_REGIONS)
    return false;
  int shared_edges[2] = {shared_edge1,shared_edge2};
  int p, shared_ends[2]; // Indices of the second vertex associated with edge
  for (p=0; p < 2; p++)
    shared_ends[p] = (shared_edges[p] & ~3) + (((shared_edges[p] & 3)+1)&3);
  assert(region_vertices[shared_edges[0]]==region_vertices[shared_ends[1]]);
  kdu_coords vertices[3] =
    { region_vertices[shared_ends[0]], region_vertices[shared_ends[1]],
      region_vertices[shared_edges[1]] };
  jpx_roi test_roi;
  test_roi.init_quadrilateral(vertices[0],vertices[1],vertices[2],vertices[2]);
  if (!test_roi.check_geometry())
    return false;
  if (check_boundary_violation(test_roi))
    return false;
  int n = num_regions++;
  int base_idx = n<<2;
  for (p=0; p < 3; p++)
    region_vertices[base_idx+p] = vertices[p];
  region_vertices[base_idx+3] = vertices[2]; // Double up last vertex
  for (p=0; p < 2; p++)
    { 
      region_edges[base_idx+p] = shared_edges[p];
      region_edges[shared_edges[p]] = base_idx+p;
    }
  region_edges[base_idx+2] = region_edges[base_idx+3] = JXPF_INTERNAL_EDGE;
  return true;
}


/* ========================================================================= */
/*                                jx_regions                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                       jx_regions::set_num_regions                         */
/*****************************************************************************/

void
  jx_regions::set_num_regions(int num)
{
  if (num < 0)
    num = 0;
  if (num <= max_regions)
    num_regions = num;
  else if (num == 1)
    {
      assert((regions == NULL) && (max_regions == 0));
      max_regions = num_regions = 1;
      regions = &bounding_region;
    }
  else
    {
      int new_max_regions = max_regions + num;
      jpx_roi *tmp = new jpx_roi[new_max_regions];
      for (int n=0; n < num_regions; n++)
        tmp[n] = regions[n];
      if ((regions != NULL) && (regions != &bounding_region))
        delete[] regions;
      regions = tmp;
      max_regions = new_max_regions;
      num_regions = num;
    }
}

/*****************************************************************************/
/*                            jx_regions::write                              */
/*****************************************************************************/

void
  jx_regions::write(jp2_output_box &box)
{
  int n, num_rois=0;
  for (n=0; (n < num_regions) && (num_rois < 255); n++, num_rois++)
    if ((regions[n].flags & JPX_QUADRILATERAL_ROI) ||
        (regions[n].is_elliptical && (regions[n].elliptical_skew.x ||
                                      regions[n].elliptical_skew.y)))
      {
        if (num_rois >= 254)
          break;
        num_rois++;
      }
  if (n < num_regions)
    { KDU_WARNING(w,0x11011002); w <<
      KDU_TXT("Cannot write all component regions to a single containing "
              "ROI Description (`roid') description box.  The JPX file "
              "format imposes a limit of 255 individual regions, but note "
              "that general quadrilaterals and oriented ellipses each "
              "consume 2 regions from this limit.");
    }
  box.write((kdu_byte) num_rois);
  jpx_roi *rp;
  for (rp=regions; n > 0; n--, rp++)
    {
      if (rp->is_elliptical)
        { // Write elliptical region
          kdu_coords centre, extent, skew;
          rp->get_ellipse(centre,extent,skew);
          box.write((kdu_byte)((rp->is_encoded)?1:0));
          box.write((kdu_byte) 1);
          box.write((kdu_byte) rp->coding_priority);
          box.write(centre.x);
          box.write(centre.y);
          box.write(extent.x);
          box.write(extent.y);
          if (skew.x || skew.y)
            {
              if (skew.x <= -extent.x) skew.x = 1-extent.x;
              if (skew.x >= extent.x) skew.x = extent.x-1;
              if (skew.y <= -extent.y) skew.y = 1-extent.y;
              if (skew.y >= extent.y) skew.y = extent.y-1;
              box.write((kdu_byte)((rp->is_encoded)?1:0));
              box.write((kdu_byte) 3);
              box.write((kdu_byte) rp->coding_priority);
              box.write(centre.x+skew.x);
              box.write(centre.y-skew.y);
              box.write((int) 1);
              box.write((int) 1);
            }
        }
      else
        { // Write rectangular region
          box.write((kdu_byte)((rp->is_encoded)?1:0));
          box.write((kdu_byte) 0);
          box.write((kdu_byte) rp->coding_priority);
          box.write(rp->region.pos.x);
          box.write(rp->region.pos.y);
          box.write(rp->region.size.x);
          box.write(rp->region.size.y);
          if (rp->flags & JPX_QUADRILATERAL_ROI)
            {
              kdu_dims inner_rect;
              int Rtyp = encode_general_quadrilateral(*rp,inner_rect);
              box.write((kdu_byte)((rp->is_encoded)?1:0));
              box.write((kdu_byte) Rtyp);
              box.write((kdu_byte) rp->coding_priority);
              box.write(inner_rect.pos.x);
              box.write(inner_rect.pos.y);
              box.write(inner_rect.size.x);
              box.write(inner_rect.size.y);
            }
        }
    }
}

/*****************************************************************************/
/*                            jx_regions::read                               */
/*****************************************************************************/

bool jx_regions::read(jp2_input_box &box)
{
  kdu_byte num=0; box.read(num);
  set_num_regions((int) num);
  int n;
  kdu_dims rect;
  kdu_coords bound_min, bound_lim, min, lim;
  for (n=0; n < num_regions; n++)
    {
      kdu_byte Rstatic, Rtyp, Rcp;
      kdu_uint32 Rlcx, Rlcy, Rwidth, Rheight;
      if (!(box.read(Rstatic) && box.read(Rtyp) &&
            box.read(Rcp) && box.read(Rlcx) && box.read(Rlcy) &&
            box.read(Rwidth) && box.read(Rheight) && (Rstatic < 2)))
        { KDU_WARNING(w,0x26100802); w <<
          KDU_TXT("Malformed ROI Descriptions (`roid') "
                  "box encountered in JPX data source.");
          return false;
        }
      rect.pos.x = (int) Rlcx;
      rect.pos.y = (int) Rlcy;
      rect.size.x = (int) Rwidth;
      rect.size.y = (int) Rheight;
      if (Rtyp == 0)
        { // Simple rectangle 
          regions[n].init_rectangle(rect,(Rstatic==1),Rcp);
        }
      else if (Rtyp == 1)
        { // Simple ellipse
          kdu_coords centre = rect.pos, extent = rect.size;
          regions[n].init_ellipse(centre,extent,kdu_coords(),
                                  (Rstatic==1),Rcp);
          rect = regions[n].region;
        }
      else if (Rtyp == 3)
        { // Oriented elliptical refinement
          kdu_coords centre, extent, skew;
          if ((n == 0) || (!regions[n-1].get_ellipse(centre,extent,skew)) ||
              (skew.x != 0) || (skew.y != 0) ||
              ((skew.x = rect.pos.x-centre.x) <= -extent.x) ||
              ((skew.y = centre.y-rect.pos.y) <= -extent.y) ||
              (skew.x >= extent.x) || (skew.y >= extent.y))
            { KDU_WARNING(w,0x0803001); w <<
              KDU_TXT("Malformed oriented elliptical region refinement "
                      "information found in ROI Descriptions (`roid') "
                      "box in JPX data source.  Oriented ellipses require "
                      "two consecutive regions, the first of which is a "
                      "simple ellipse, while the second has a centre which "
                      "lies within (but not on) the bounding rectangle "
                      "associated with the first ellipse.");
              return false;
            }
          num_regions--; n--;
          regions[n].init_ellipse(centre,extent,skew,regions[n].is_encoded,
                                  regions[n].coding_priority);
          rect = regions[n].region;
        }
      else if (((Rtyp & 1) == 0) && ((Rtyp & 7) >= 2) && ((Rtyp & 48) <= 32))
        { // Quadrilateral refinement
          kdu_dims outer_rect;
          if ((n == 0) || (!regions[n-1].get_rectangle(outer_rect)) ||
              ((rect & outer_rect) != rect))
            { KDU_WARNING(w,0x09011001); w <<
              KDU_TXT("Malformed quadrilateral region of interest "
                      "information found in ROI Descriptions (`roid') "
                      "box in JPX data source.  Generic "
                      "quadrilaterals require two consecutive, "
                      "regions, the first of which is a simple rectangle, "
                      "while the second identifies a second rectangular "
                      "region, contained within the first.");
              return false;
            }
          num_regions--; n--;
          int A = (Rtyp >> 6) & 3;
          int B = (Rtyp >> 4) & 3;
          int C = (Rtyp >> 3) & 1;
          int D = (Rtyp >> 1) & 3;
          if (!promote_roi_to_general_quadrilateral(regions[n],rect,A,B,C,D))
            return false;
          continue;
        }
      else
        { // Skip over unrecognized region type codes to allow for future
          // enhancements.
          num_regions--; n--; continue;
        }
      min = rect.pos;
      lim = min + rect.size;
      if (n == 0)
        { bound_min = min; bound_lim = lim; }
      else
        { // Grow `bound' to include `rect'
          if (min.x < bound_min.x)
            bound_min.x = min.x;
          if (min.y < bound_min.y)
            bound_min.y = min.y;
          if (lim.x > bound_lim.x)
            bound_lim.x = lim.x;
          if (lim.y > bound_lim.y)
            bound_lim.y = lim.y;
        }
    }
  bounding_region.region.pos = bound_min;
  bounding_region.region.size = bound_lim - bound_min;
  double wd_max = 0.1;
  for (n=0; n < num_regions; n++)
    {
      double dummy, wd=0.0;
      regions[n].measure_span(wd,dummy);
      wd_max = (wd > wd_max)?wd:wd_max;
    }
  max_width = (int) ceil(wd_max);
  return true;
}

/*****************************************************************************/
/* STATIC      jx_regions::promote_roi_to_general_quadrilateral              */
/*****************************************************************************/

bool
  jx_regions::promote_roi_to_general_quadrilateral(jpx_roi &roi,
                                                   kdu_dims inner_rect,
                                                   int A, int B, int C, int D)
{
  A &= 3; B &= 3; D &= 3; C &= 1; // Just to make sure there are no disasters
  kdu_coords v[4];
  v[0] = roi.region.pos;
  v[1] = inner_rect.pos;
  v[2] = v[1] + inner_rect.size - kdu_coords(1,1);
  v[3] = v[0] + roi.region.size - kdu_coords(1,1);
  assert((v[0].x <= v[1].x) && (v[0].y <= v[1].y) &&
         (v[1].x <= v[2].x) && (v[1].y <= v[2].y) &&
         (v[2].x <= v[3].x) && (v[2].y <= v[3].y));

  int B1 = B+1+C; if (B1 > 2) B1-=3;
  int B2 = B+2-C; if (B2 > 2) B2-=3;
  int a=A, b=(A+1+B)&3, c=(A+1+B1)&3, d=(A+1+B2)&3;
  int x[4] = {v[0].x,v[1].x,v[2].x,v[3].x};
  v[0].x = x[a]; v[1].x = x[b]; v[2].x = x[c]; v[3].x = x[d];
  
  roi.vertices[0] = v[0];
  switch (D) {
    case 1:
      roi.vertices[1]=v[1]; roi.vertices[2]=v[2]; roi.vertices[3]=v[3];
      break;
    case 2:
      roi.vertices[1]=v[2]; roi.vertices[2]=v[3]; roi.vertices[3]=v[1];
      break;      
    case 3:
      roi.vertices[1]=v[3]; roi.vertices[2]=v[1]; roi.vertices[3]=v[2];
      break;
    default: return false;
  };
  roi.flags = JPX_QUADRILATERAL_ROI;
  if (roi.check_geometry())
    return true;
  
  // Reverse the vertex order, keeping the top vertex at index 0
  kdu_coords tmp = roi.vertices[1];
  roi.vertices[1] = roi.vertices[3];
  roi.vertices[3] = tmp;
  if (!roi.check_geometry())
    { KDU_WARNING(w,0x11011001); w <<
      KDU_TXT("Illegal quadrilateral vertices encountered while reading an "
              "ROI Description (`roid') box from a JPX source.  Quadrilateral "
              "edges cross through each other!");
      return false;
    }
  return true;
}

/*****************************************************************************/
/* STATIC           jx_regions::encode_general_quadrilateral                 */
/*****************************************************************************/

int
  jx_regions::encode_general_quadrilateral(jpx_roi &roi, kdu_dims &inner_rect)
{
  kdu_coords tmp, v[4] =
    { roi.vertices[0],roi.vertices[1],roi.vertices[2],roi.vertices[3] };
  
  // Start by sorting the vertices into increasing order of their Y coordinate,
  // keeping track of the connectivity.  Note that v[0] is already the
  // top-most vertex so it will not move.
  
  assert((v[0].y <= v[1].y) && (v[0].y <= v[2].y) && (v[0].y <= v[3].y));
  int n, opp_idx=2; // v[opp_idx] is the vertex opposite v[0]
  bool done = false;
  while (!done)
    { // Perform bubble sort on the y coordinate
      done = true;
      for (n=1; n < 3; n++)
        if (v[n].y > v[n+1].y)
          { // Swap v[n] with v[n+1]
            done = false;
            if (n == opp_idx)
              opp_idx = n+1;
            else if ((n+1) == opp_idx)
              opp_idx = n;
            tmp = v[n]; v[n] = v[n+1]; v[n+1] = tmp;
          }
    }
  
  // Now find the sequence of the x coordinates
  int seq[4]={0,1,2,3};
  done = false;
  while (!done)
    { // Perform bubble sort on the x coordinate
      done = true;
      for (n=0; n < 3; n++)
        if (v[seq[n]].x > v[seq[n+1]].x)
          { 
            done = false;
            int t=seq[n]; seq[n] = seq[n+1]; seq[n+1] = t;
          }
    }
  
  // Now find the bit-fields and inner rectangle
  inner_rect.pos.y = v[1].y;
  inner_rect.size.y = v[2].y + 1 - v[1].y;
  inner_rect.pos.x = v[seq[1]].x;
  inner_rect.size.x = v[seq[2]].x + 1 - v[seq[1]].x;
  int iseq[4]={0,0,0,0}; // Reverse permutation
  for (n=0; n < 4; n++)
    iseq[seq[n]] = n;
  int A = iseq[0];
  int B = ((iseq[1]-iseq[0])&3) - 1;
  int C = ((iseq[2]-iseq[0])&3) - ((iseq[1]-iseq[0])&3); C=(C<0)?(C+2):(C-1);
  int D = (opp_idx==1)?3:(opp_idx-1);
  return (A<<6) + (B<<4) + (C<<3) + (D<<1);
}


/* ========================================================================= */
/*                               jx_crossref                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                          jx_crossref::link_found                          */
/*****************************************************************************/

void jx_crossref::link_found()
{
  assert(metaloc != NULL);
  link = metaloc->target;
  if ((link != NULL) && (link_type == JPX_METANODE_LINK_NONE))
    { // Determine the link type
      if (owner->flags & JX_METANODE_FIRST_SUBBOX)
        link_type = JPX_GROUPING_LINK;
      else if (link->flags & JX_METANODE_FIRST_SUBBOX)
        { // Might be an alterate parent link; need to determine whether the
          // cross-reference referred to the asoc box itself or the first
          // sub-box.
          jx_metaloc_manager &metaloc_manager=owner->manager->metaloc_manager;
          kdu_long asoc_pos = metaloc->get_loc()-8;
          jx_metaloc *asoc_metaloc=metaloc_manager.get_locator(asoc_pos,false);
          if (asoc_metaloc == NULL)
            asoc_metaloc = metaloc_manager.get_locator(asoc_pos-=8,false);
          if ((asoc_metaloc != NULL) &&
              (asoc_metaloc->target ==metaloc->target))
            link_type = JPX_ALTERNATE_PARENT_LINK;
          else
            link_type = JPX_ALTERNATE_CHILD_LINK;
        }
      else
        link_type = JPX_ALTERNATE_CHILD_LINK;
    }
  metaloc = NULL;
  owner->append_to_touched_list(true); // Touches descendants of grouping links
  owner->manager->have_link_nodes = true;
}

/*****************************************************************************/
/*                            jx_crossref::unlink                            */
/*****************************************************************************/

void jx_crossref::unlink()
{
  jx_crossref *scan, *prev;
  if (link != NULL)
    { // We must be on a list headed by `link->linked_from'
      scan = link->linked_from;
    }
  else if (metaloc != NULL)
    { // We must be on a list headed by `metaloc->target->crossref'
      scan = metaloc->target->crossref;
      assert((scan->link == NULL) && (scan->metaloc == metaloc));
    }
  else
    return;
  for (prev=NULL; scan != NULL; prev=scan, scan=scan->next_link)
    if (scan == this)
      {
        if (prev == NULL)
          { // Change the head of the list
            if (link != NULL)
              link->linked_from = scan->next_link;
            else if (scan->next_link != NULL)
              metaloc->target = scan->next_link->owner;
            else
              metaloc->target = NULL;
          }
        else
          prev->next_link = scan->next_link;
        break;
      }
  assert(scan != NULL);
  link = NULL;
  metaloc = NULL;
  next_link = NULL;
}


/* ========================================================================= */
/*                               jx_metanode                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                        jx_metanode::~jx_metanode                          */
/*****************************************************************************/

jx_metanode::~jx_metanode()
{
  assert((flags & JX_METANODE_DELETED) &&
         (head == NULL) && (tail == NULL) && (parent == NULL) &&
         (metagroup == NULL) && (next_link == NULL) && (prev_link == NULL) &&
         (linked_from == NULL) &&
         (prev_sibling == NULL)); // We only use `next_sibling' to link the
                                  // `deleted_nodes' list.
  switch (rep_id) {
    case JX_REF_NODE: if (ref != NULL) delete ref; break;
    case JX_NUMLIST_NODE: if (numlist != NULL) delete numlist; break;
    case JX_ROI_NODE: if (regions != NULL) delete regions; break;
    case JX_LABEL_NODE: if (label != NULL) delete label; break;
    case JX_CROSSREF_NODE: if (crossref != NULL) delete crossref; break;
    case JX_NULL_NODE: break;
    default: assert(0); break;
    };

  if ((flags & JX_METANODE_EXISTING) && (read_state != NULL))
    delete read_state;
  else if (write_state != NULL)
    delete write_state;
}

/*****************************************************************************/
/*                       jx_metanode::safe_delete                           */
/*****************************************************************************/

void
  jx_metanode::safe_delete()
{
  if (flags & JX_METANODE_DELETED)
    return; // Already deleted
  flags |= JX_METANODE_DELETED; // Avoid any chance of recursive entry here
  unlink_parent();
  if (metagroup != NULL)
    metagroup->unlink(this);
  assert((next_link == NULL) && (prev_link == NULL) && (metagroup == NULL) &&
         (parent == NULL) && (next_sibling == NULL) && (prev_sibling == NULL));
  
  if ((rep_id == JX_CROSSREF_NODE) && (crossref != NULL))
    {
      delete crossref; // Automatically unlinks it
      crossref = NULL;
    }

  append_to_touched_list(false);
  
  // Now apply `safe_delete' to all nodes which link to us
  while (linked_from != NULL)
    linked_from->owner->safe_delete();
  
  // Now recursively apply `safe_delete' to all descendants
  while (head != NULL)
    head->safe_delete();
  
  assert((tail == NULL) && (num_descendants == 0));
  next_sibling = manager->deleted_nodes;
  manager->deleted_nodes = this;
}

/*****************************************************************************/
/*                       jx_metanode::unlink_parent                          */
/*****************************************************************************/

void
  jx_metanode::unlink_parent()
{
  if (parent != NULL)
    { // Unlink from sibling list
      if (prev_sibling == NULL)
        {
          assert(this == parent->head);
          parent->head = next_sibling;
        }
      else
        prev_sibling->next_sibling = next_sibling;
      if (next_sibling == NULL)
        {
          assert(this == parent->tail);
          parent->tail = prev_sibling;
        }
      else
        next_sibling->prev_sibling = prev_sibling;
      parent->num_descendants--;
      if (flags & JX_METANODE_IS_COMPLETE)
        parent->num_completed_descendants--;
      parent = next_sibling = prev_sibling = NULL;
    }
  else
    assert((next_sibling == NULL) && (prev_sibling == NULL));  
}

/*****************************************************************************/
/*                  jx_metanode::append_to_touched_list                      */
/*****************************************************************************/

void
  jx_metanode::append_to_touched_list(bool recursive)
{
  if ((box_type != 0) && (flags & JX_METANODE_BOX_COMPLETE))
    {
      // Unlink from touched list if required
      if (manager->touched_head == this)
        {
          assert(prev_touched == NULL);
          manager->touched_head = next_touched;
        }
      else if (prev_touched != NULL)
        prev_touched->next_touched = next_touched;
      if (manager->touched_tail == this)
        {
          assert(next_touched == NULL);
          manager->touched_tail = prev_touched;
        }
      else if (next_touched != NULL)
        next_touched->prev_touched = prev_touched;
  
      // Now link onto tail of touched list
      next_touched = NULL;
      if ((prev_touched = manager->touched_tail) == NULL)
        {
          assert(manager->touched_head == NULL);
          manager->touched_head = manager->touched_tail = this;
        }
      else
        manager->touched_tail = manager->touched_tail->next_touched = this;
    }

  if ((parent != NULL) && (parent->flags & (JX_METANODE_ANCESTOR_CHANGED |
                                            JX_METANODE_CONTENTS_CHANGED)))
    flags |= JX_METANODE_ANCESTOR_CHANGED;
  
  jx_metanode *scan;
  for (scan=head; (scan != NULL) && recursive; scan=scan->next_sibling)
    scan->append_to_touched_list(true);
}

/*****************************************************************************/
/*                   jx_metanode::place_on_touched_list                      */
/*****************************************************************************/

void jx_metanode::place_on_touched_list()
{
  if ((box_type != 0) && (flags & JX_METANODE_BOX_COMPLETE))
    {
      // Check if we are already on the touched list
      if ((manager->touched_head == this) || (prev_touched != NULL))
        return;
      assert((prev_touched == NULL) && (next_touched == NULL) &&
             (manager->touched_tail != this));
      
      // Link onto tail of touched list
      next_touched = NULL;
      if ((prev_touched = manager->touched_tail) == NULL)
        {
          assert(manager->touched_head == NULL);
          manager->touched_head = manager->touched_tail = this;
        }
      else
        manager->touched_tail = manager->touched_tail->next_touched = this;
    }
}

/*****************************************************************************/
/*                       jx_metanode::append_child                           */
/*****************************************************************************/

void
  jx_metanode::append_child(jx_metanode *child)
{
  child->prev_sibling = tail;
  if (tail == NULL)
    head = tail = child;
  else
    tail = tail->next_sibling = child;
  child->parent = this;
  num_descendants++;
  if (child->flags & JX_METANODE_IS_COMPLETE)
    num_completed_descendants++;
}

/*****************************************************************************/
/*                       jx_metanode::add_descendant                         */
/*****************************************************************************/

jx_metanode *
  jx_metanode::add_descendant()
{
  jx_metanode *result = new jx_metanode(manager);
  append_child(result);
  return result;
}

/*****************************************************************************/
/*                         jx_metanode::add_numlist                          */
/*****************************************************************************/

jx_metanode *
  jx_metanode::add_numlist(int num_codestreams, const int *codestream_indices,
                           int num_layers, const int *layer_indices,
                           bool applies_to_rendered_result)
{
  jx_metanode *node = add_descendant();  
  node->box_type = jp2_number_list_4cc;
  node->flags |= JX_METANODE_BOX_COMPLETE;
  node->rep_id = JX_NUMLIST_NODE;
  node->numlist = new jx_numlist;
  int n;
  if (num_codestreams > 0)
    {
      node->numlist->num_codestreams =
        node->numlist->max_codestreams = num_codestreams;
      node->numlist->codestream_indices = new int[num_codestreams];
      for (n=0; n < num_codestreams; n++)
        node->numlist->codestream_indices[n] = codestream_indices[n];
    }
  if (num_layers > 0)
    {
      node->numlist->num_compositing_layers =
        node->numlist->max_compositing_layers = num_layers;
      node->numlist->layer_indices = new int[num_layers];
      for (n=0; n < num_layers; n++)
        node->numlist->layer_indices[n] = layer_indices[n];
    }
  node->numlist->rendered_result = applies_to_rendered_result;
  node->manager->link(node);
  node->append_to_touched_list(false);
  return node;
}

/*****************************************************************************/
/*                jx_metanode::update_completed_descendants                  */
/*****************************************************************************/

void
  jx_metanode::update_completed_descendants()
{
  assert((flags & JX_METANODE_DESCENDANTS_KNOWN) &&
         (flags & JX_METANODE_BOX_COMPLETE));
  jx_metanode *scan = this;
  while (((scan->flags & JX_METANODE_IS_COMPLETE) == 0) &&
         (scan->flags & JX_METANODE_DESCENDANTS_KNOWN) &&
         (scan->num_completed_descendants == scan->num_descendants))
    {
      scan->flags |= JX_METANODE_IS_COMPLETE;
      if (scan->parent == NULL)
        break;
      assert(scan->parent->num_completed_descendants <
             scan->parent->num_descendants);
      scan->parent->num_completed_descendants++;
      scan = scan->parent;
    }
}

/*****************************************************************************/
/*                      jx_metanode::donate_input_box                        */
/*****************************************************************************/

void
  jx_metanode::donate_input_box(jp2_input_box &src, int databin_nesting_level)
{
  assert((read_state == NULL) &&
         !(flags & (JX_METANODE_BOX_COMPLETE|JX_METANODE_DESCENDANTS_KNOWN)));
  flags |= JX_METANODE_EXISTING;
  box_type = src.get_box_type();
  if (box_type == jp2_association_4cc)
    flags |= JX_METANODE_FIRST_SUBBOX;
  
  // Add a locator for the new box.
  if (box_type != jp2_cross_reference_4cc)
    {
      kdu_long file_pos = src.get_locator().get_file_pos();
      file_pos += src.get_box_header_length();
      jx_metaloc *metaloc=manager->metaloc_manager.get_locator(file_pos,true);
      if ((metaloc->target != NULL) &&
          (metaloc->target->rep_id == JX_CROSSREF_NODE) &&
          (metaloc->target->crossref->metaloc == metaloc))
        linked_from = metaloc->target->crossref;
      metaloc->target = this;
      for (jx_crossref *scan=linked_from; scan != NULL; scan=scan->next_link)
        scan->link_found();
    }
  
  read_state = new jx_metaread();
  if (box_type == jp2_association_4cc)
    {
      jp2_locator loc = src.get_locator();
      read_state->asoc_databin_id = loc.get_databin_id();
      read_state->asoc_nesting_level = databin_nesting_level;
      loc = src.get_contents_locator();
      read_state->box_databin_id = loc.get_databin_id();
      if (read_state->box_databin_id == read_state->asoc_databin_id)
        read_state->box_nesting_level = databin_nesting_level+1;
      else
        read_state->box_nesting_level = 0;
      read_state->asoc.transplant(src);
      box_type = 0; // Because we don't yet know the node's box-type
    }
  else
    {
      jp2_locator loc = src.get_locator();
      read_state->asoc_databin_id = -1;
      read_state->box_databin_id = loc.get_databin_id();
      read_state->box_nesting_level = databin_nesting_level;
      read_state->box.transplant(src);
      flags |= JX_METANODE_DESCENDANTS_KNOWN;
    }
  finish_reading();
}

/*****************************************************************************/
/*                       jx_metanode::finish_reading                         */
/*****************************************************************************/

bool
  jx_metanode::finish_reading()
{
  if (!(flags & JX_METANODE_EXISTING))
    return true;
  if (read_state == NULL)
    {
      assert((flags & JX_METANODE_BOX_COMPLETE) &&
             (flags & JX_METANODE_DESCENDANTS_KNOWN));
      return true;
    }

  if (!(flags & JX_METANODE_BOX_COMPLETE))
    {
      jx_crossref *cref;
      if (!read_state->box)
        { // We still have to open the first sub-box
          assert(read_state->asoc.exists());
          read_state->box.open(&read_state->asoc);
          if (!read_state->box)
            return false;
          box_type = read_state->box.get_box_type();

          // Add a locator for the newly opened box
          if (box_type != jp2_cross_reference_4cc)
            {
              kdu_long file_pos =
                read_state->box.get_locator().get_file_pos();
              file_pos += read_state->box.get_box_header_length();
              jx_metaloc *metaloc =
                manager->metaloc_manager.get_locator(file_pos,true);
              jx_crossref *new_linkers = NULL;
              if ((metaloc->target != NULL) &&
                  (metaloc->target->rep_id == JX_CROSSREF_NODE) &&
                  (metaloc->target->crossref->metaloc == metaloc))
                {
                  new_linkers = metaloc->target->crossref;
                  if (linked_from == NULL)
                    linked_from = new_linkers;
                  else
                    {
                      jx_crossref *lfs=linked_from;
                      for (; lfs->next_link != NULL; lfs=lfs->next_link);
                      lfs->next_link = new_linkers;
                    }
                }
              metaloc->target = this;
              for (cref=new_linkers; cref != NULL; cref=cref->next_link)
                cref->link_found();
            }
        }
      if (!read_state->box.is_complete())
        return false;
          
      flags |= JX_METANODE_BOX_COMPLETE;
      if (parent != NULL)
        parent->place_on_touched_list();
      append_to_touched_list(false); // Force to end of list
      for (cref=linked_from; cref != NULL; cref=cref->next_link)
        cref->owner->append_to_touched_list(true);
          // Force descendants of newly connected linkers to end of list
      int len = (int) read_state->box.get_remaining_bytes();
      if (box_type == jp2_number_list_4cc)
        {
          rep_id = JX_NUMLIST_NODE;
          numlist = new jx_numlist;
          if ((len == 0) || ((len & 3) != 0))
            { KDU_WARNING(w,0x26100801); w <<
              KDU_TXT("Malformed Number List (`nlst') box "
                      "found in JPX data source.  Box body does not "
                      "contain a whole (non-zero) number of 32-bit "
                      "integers.");
              safe_delete();
              return true;
            }
          len >>= 2;
          for (; len > 0; len--)
            {
              kdu_uint32 idx; read_state->box.read(idx);
              if (idx == 0)
                numlist->rendered_result = true;
              else if (idx & 0x01000000)
                numlist->add_codestream((int)(idx & 0x00FFFFFF));
              else if (idx & 0x02000000)
                numlist->add_compositing_layer((int)(idx & 0x00FFFFFF));
            }
        }
      else if ((box_type == jp2_roi_description_4cc) && (len > 1))
        {
          rep_id = JX_ROI_NODE;
          regions = new jx_regions;
          if (!regions->read(read_state->box))
            {
              safe_delete();
              return true;
            }
        }
      else if ((box_type == jp2_label_4cc) && (len < 8192))
        {
          label = new char[len+1];
          rep_id = JX_LABEL_NODE;
          read_state->box.read((kdu_byte *) label,len);
          label[len] = '\0';
        }
      else if (box_type == jp2_cross_reference_4cc)
        {
          rep_id = JX_CROSSREF_NODE;
          crossref = new jx_crossref;
          crossref->owner = this;
          jp2_input_box flst;
          bool success = (read_state->box.read(crossref->box_type) &&
                          flst.open(&(read_state->box)) &&
                          (flst.get_box_type() == jp2_fragment_list_4cc) &&
                          crossref->frag_list.init(&flst,false));
          flst.close();
          if (!success)
            { KDU_WARNING(w,0x19040901); w <<
              KDU_TXT("Malformed Cross Reference (`cref') box "
                      "found in JPX data source.");
              safe_delete();
              return true;
            }
          if (manager->test_box_filter(crossref->box_type))
            {
              kdu_long link_pos, link_len;
              link_pos = crossref->frag_list.get_link_region(link_len);
              if (link_pos > 0)
                {
                  jx_metaloc *metaloc = // Set `crossref->metaloc' later
                  manager->metaloc_manager.get_locator(link_pos,true);
                  if (metaloc->target == NULL)
                    { // Temporarily make `metaloc' point to ourselves
                      crossref->metaloc = metaloc;
                      metaloc->target = this;
                    }
                  else if ((metaloc->target->rep_id == JX_CROSSREF_NODE) &&
                           (metaloc->target->crossref != NULL) &&
                           (metaloc->target->crossref->metaloc == metaloc))
                    {
                      crossref->append_to_list(metaloc->target->crossref);
                      crossref->metaloc = metaloc;
                    }
                  else if (metaloc->target->flags & JX_METANODE_DELETED)
                    { // We will not be able to complete the link
                      safe_delete();
                      return true;
                    }
                  else
                    {
                      if (metaloc->target->linked_from == NULL)
                        metaloc->target->linked_from = crossref;
                      else
                        crossref->append_to_list(metaloc->target->linked_from);
                      crossref->metaloc = metaloc; // So `link_found' works
                      crossref->link_found();
                    }
                }
            }
        }
      else if (box_type == jp2_uuid_4cc)
        {
          rep_id = JX_REF_NODE;
          memset(data,0,16);
          read_state->box.read(data,16);
          ref = new jx_metaref;
          ref->src = manager->ultimate_src;
          ref->src_loc = read_state->box.get_locator();
        }
      else
        {
          rep_id = JX_REF_NODE;
          ref = new jx_metaref;
          ref->src = manager->ultimate_src;
          ref->src_loc = read_state->box.get_locator();
        }
      read_state->box.close();
      manager->link(this);
      if (flags & JX_METANODE_DESCENDANTS_KNOWN)
        update_completed_descendants();
    }
  
  while (!(flags & JX_METANODE_DESCENDANTS_KNOWN))
    {
      if (read_state->codestream_source != NULL)
        {
          if (!read_state->codestream_source->finish(false))
            return false;
          continue;
        }
      if (read_state->layer_source != NULL)
        {
          if (!read_state->layer_source->finish())
            return false;
          continue;
        }
      assert(read_state->asoc.exists());
      assert(!read_state->box);
      if (read_state->box.open(&read_state->asoc))
        {
          if (manager->test_box_filter(read_state->box.get_box_type()))
            {
              jx_metanode *node = add_descendant();
              node->donate_input_box(read_state->box,
                                     read_state->box_nesting_level);
            }
          else
            read_state->box.close(); // Skip this sub-box
          if (read_state->asoc.get_remaining_bytes() == 0)
            { // Test may succeed, even if `read_state->asoc.is_complete'
              // returns false, because the last sub-box of the asoc box
              // may not be completely available in the cache -- that is
              // no longer our concern, since we now know where all our
              // descendants are, but the tests below would fail to
              // discover that the descendants are all known -- corrected
              // the oversight in v6.2
              flags |= JX_METANODE_DESCENDANTS_KNOWN;
              this->place_on_touched_list();
              update_completed_descendants();
            }
        }
      else if (!read_state->asoc.is_complete())
        return false;
      else
        {
          flags |= JX_METANODE_DESCENDANTS_KNOWN;
          this->place_on_touched_list();
          update_completed_descendants();
        }
    }

  delete read_state;
  read_state = NULL;
  return true;
}

/*****************************************************************************/
/*                        jx_metanode::load_recursive                        */
/*****************************************************************************/

void
  jx_metanode::load_recursive()
{
  if ((flags & JX_METANODE_EXISTING) && (read_state != NULL) &&
      !((flags & JX_METANODE_BOX_COMPLETE) &&
        (flags & JX_METANODE_DESCENDANTS_KNOWN)))
    finish_reading();

  for (jx_metanode *scan=head;
       (scan != NULL) && (num_completed_descendants != num_descendants);
       scan=scan->next_sibling)
    scan->load_recursive();
}

/*****************************************************************************/
/*                         jx_metanode::find_path_to                         */
/*****************************************************************************/

jx_metanode *
  jx_metanode::find_path_to(jx_metanode *tgt, int descending_flags,
                            int ascending_flags, int num_excl,
                            const kdu_uint32 *excl_types,
                            const int *excl_flags,
                            bool unify_groups)
{
  assert((flags & JX_METANODE_LOOP_DETECTION) == 0);
  if (this == tgt)
    return this;
  if (unify_groups)
    {
      jx_metanode *alt_tgt=NULL, *alt_this=NULL;
      if ((tgt->rep_id == JX_CROSSREF_NODE) && (tgt->crossref != NULL) &&
          (tgt->crossref->link_type == JPX_GROUPING_LINK) &&
          (tgt->crossref->link != NULL) &&
          (tgt->crossref->link->flags & JX_METANODE_BOX_COMPLETE))
        alt_tgt = tgt->crossref->link;
      if ((this->rep_id == JX_CROSSREF_NODE) && (this->crossref != NULL) &&
          (this->crossref->link_type == JPX_GROUPING_LINK) &&
          (this->crossref->link != NULL) &&
          (this->crossref->link->flags & JX_METANODE_BOX_COMPLETE))
        alt_this = this->crossref->link;
      if ((alt_tgt == this) || (alt_this == tgt) ||
          ((alt_tgt != NULL) && (alt_tgt == alt_this)))
        return this;
    }

  if (!(flags & JX_METANODE_BOX_COMPLETE))
    return NULL;
  
  this->flags |= JX_METANODE_LOOP_DETECTION; // Mark this node to detect cycles
  
  // Consider parent
  if ((parent != NULL) && (ascending_flags & JPX_PATH_TO_DIRECT))
    {
      if ((parent == tgt) ||
          ((!(parent->flags & JX_METANODE_LOOP_DETECTION)) &&
           (parent->find_path_to(tgt,0,ascending_flags,num_excl,
                                 excl_types,excl_flags,unify_groups) != NULL)))
        { // Found a path; first segment of path is `parent'
          this->flags &= ~JX_METANODE_LOOP_DETECTION;
          return parent;
        }
    }
  
  // Consider descendants as direct matches or forward links
  int direct_descent = (descending_flags & JPX_PATH_TO_DIRECT);
  int forward_descent = (descending_flags & JPX_PATH_TO_FORWARD);
  int forward_ascent = (ascending_flags & JPX_PATH_TO_FORWARD);
  if (direct_descent || forward_descent || forward_ascent)
    {
      jx_metanode *scan;
      for (scan=head; scan != NULL; scan=scan->next_sibling)
        {
          if (scan == tgt)
            break; // Found a match
          if (scan->flags & JX_METANODE_LOOP_DETECTION)
            continue;
          if ((scan->num_descendants > 0) && direct_descent &&
              (scan->find_path_to(tgt,descending_flags,ascending_flags,
                                  num_excl,excl_types,excl_flags,
                                  unify_groups) != NULL))
            break; // Found a path; first segment of path is `scan'
          if ((scan->rep_id == JX_CROSSREF_NODE) && (scan->crossref != NULL) &&
              (scan->crossref->link != NULL))
            {
              jx_metanode *link = scan->crossref->link;
              if (link->flags & JX_METANODE_LOOP_DETECTION)
                continue; // We have visited this link before!
              if ((num_excl > 0) &&
                  link->check_path_exclusion(num_excl,excl_types,excl_flags))
                continue; // We are about to visit an excluded link
              if (forward_descent &&
                  (scan->crossref->link_type == JPX_ALTERNATE_CHILD_LINK) &&
                  ((link == tgt) ||
                   (link->find_path_to(tgt,descending_flags,ascending_flags,
                                       num_excl,excl_types,excl_flags,
                                       unify_groups)!=NULL)))
                { // Found a path; first segment of path is `link'
                  scan = link; break;
                }
              if (forward_ascent &&
                  (scan->crossref->link_type == JPX_ALTERNATE_PARENT_LINK) &&
                  ((link == tgt) ||
                   (link->find_path_to(tgt,0,ascending_flags,num_excl,
                                       excl_types,excl_flags,
                                       unify_groups) != NULL)))
                { // Found a path; first segment of path is `link'
                  scan = link; break;
                }
            }
        }
      if (scan != NULL)
        { // Found a path; remove the LOOP_DETECTION flag before returning
          this->flags &= ~JX_METANODE_LOOP_DETECTION;
          return scan; 
        }
    }

  // Consider reverse links
  int reverse_descent = (descending_flags & JPX_PATH_TO_REVERSE);
  int reverse_ascent = (ascending_flags & JPX_PATH_TO_REVERSE);
  if (reverse_descent || reverse_ascent)
    {
      jx_crossref *scan;
      for (scan=linked_from; scan != NULL; scan=scan->next_link)
        {
          jx_metanode *linker = scan->owner;
          if (linker->flags & JX_METANODE_LOOP_DETECTION)
            continue; // We have visited this linker before!
          if ((num_excl > 0) &&
              linker->check_path_exclusion(num_excl,excl_types,excl_flags))
            continue; // We are about to visit an excluded linker
          if (reverse_descent &&
              (scan->link_type == JPX_ALTERNATE_PARENT_LINK) &&
              ((linker == tgt) ||
               (linker->find_path_to(tgt,descending_flags,ascending_flags,
                                     num_excl,excl_types,excl_flags,
                                     unify_groups) != NULL)))
            break; // Found a path
          if (reverse_ascent &&
              (scan->link_type == JPX_ALTERNATE_CHILD_LINK) &&
              ((linker == tgt) ||
               (linker->find_path_to(tgt,0,ascending_flags,num_excl,
                                     excl_types,excl_flags,
                                     unify_groups) != NULL)))
            break; // Found a path
        }
      if (scan != NULL)
        { // Found a path; remove the LOOP_DETECTION flag before returning
          this->flags &= ~JX_METANODE_LOOP_DETECTION;
          return scan->owner;
        }
    }
  
  this->flags &= ~JX_METANODE_LOOP_DETECTION;
  return NULL;
}

/*****************************************************************************/
/*                     jx_metanode::check_path_exclusion                     */
/*****************************************************************************/

bool
  jx_metanode::check_path_exclusion(int num_exclusion_types,
                                    const kdu_uint32 *exclusion_box_types,
                                    const int *exclusion_flags)
{
  int n;
  for (n=0; n < num_exclusion_types; n++)
    {
      kdu_uint32 test_box_type = exclusion_box_types[n];
      if ((exclusion_flags[n] & JPX_PATH_TO_EXCLUDE_BOX) &&
          (this->box_type == test_box_type))
        return true;
      if (exclusion_flags[n] & JPX_PATH_TO_EXCLUDE_PARENTS)
        for (jx_metanode *scan=parent; scan != NULL; scan=scan->parent)
          if (scan->box_type == test_box_type)
            return true;
    }
  return false;
}

/*****************************************************************************/
/*                      jx_metanode::clear_write_state                       */
/*****************************************************************************/

void
  jx_metanode::clear_write_state(bool clear_frag_lists)
{
  flags &= ~JX_METANODE_WRITTEN;
  if (write_state != NULL)
    {
      delete write_state;
      write_state = NULL;
    }
  if (clear_frag_lists && (rep_id == JX_CROSSREF_NODE) && (crossref != NULL))
    {
      crossref->box_type = 0;
      crossref->frag_list.reset();
    }
  jx_metanode *scan;
  for (scan=head; scan != NULL; scan=scan->next_sibling)
    scan->clear_write_state(clear_frag_lists);
}

/*****************************************************************************/
/*                      jx_metanode::mark_for_writing                        */
/*****************************************************************************/

bool
  jx_metanode::mark_for_writing(jx_metagroup *context)
{
  bool marked_something = false;
  jx_metanode *scan;
  for (scan=head; scan != NULL; scan=scan->next_sibling)
    if (scan->mark_for_writing(context))
      marked_something = true;
  
  if (!marked_something)
    { // See if this node needs to be written, even though it has no
      // descendants which need to be written.
      if ((flags & JX_METANODE_WRITTEN) ||
          ((write_state != NULL) && (write_state->context != NULL)))
        return false; // Already written
      if (box_type == jp2_number_list_4cc)
        { // There is no need to write a numlist which is not associated
          // with any descendants or parents.  Let's see if this is the case.
          for (scan=parent; scan != NULL; scan=scan->parent)
            if ((scan->box_type != 0) && (scan->box_type != jp2_free_4cc) &&
                (scan->box_type != jp2_number_list_4cc))
              break;
          if (scan == NULL)
            return false;
        }
    }
  if (write_state == NULL)
    {
      write_state = new jx_metawrite;
      flags &= ~JX_METANODE_WRITTEN;
    }
  assert(!(flags & JX_METANODE_WRITTEN));
  write_state->context = context;
  assert((write_state->active_descendant == NULL) &&
         (!write_state->asoc) && (!write_state->box));
  return true;
}

/*****************************************************************************/
/*                            jx_metanode::write                             */
/*****************************************************************************/

jp2_output_box *
  jx_metanode::write(jp2_output_box *super_box, jx_target *target,
                     jx_metagroup *context, int *i_param, void **addr_param)
{
  if ((write_state == NULL) || (write_state->context != context))
    return NULL;

  assert(!(flags & JX_METANODE_WRITTEN));
  
  bool is_top_box = (super_box == NULL);
  
  if ((box_type != 0) &&
      !(write_state->asoc.exists() || write_state->box.exists()))
    { // Haven't started writing anything yet.
      assert(parent != NULL);
      bool need_asoc_box = false;
      if ((box_type != jp2_free_4cc) && (num_descendants > 0))
        need_asoc_box = true;
      else if ((rep_id == JX_CROSSREF_NODE) &&
               (crossref->link_type == JPX_GROUPING_LINK))
        need_asoc_box = true;
      else
        {
          jx_crossref *cref;
          for (cref=linked_from; cref != NULL; cref=cref->next_link)
            if (cref->link_type == JPX_ALTERNATE_PARENT_LINK)
              { need_asoc_box = true; break; }
        }
      if (need_asoc_box)
        {
          if (super_box != NULL)
            write_state->asoc.open(super_box,jp2_association_4cc);
          else
            target->open_top_box(&write_state->asoc,jp2_association_4cc,
                                 manager->simulate_write);
        }
    }
  if (write_state->asoc.exists())
    super_box = &write_state->asoc;

  if (write_state->active_descendant == NULL)
    { // See if we need to write the box contents
      if (write_state->box.exists())
        { // Must be returning after having the application complete this box
          assert((rep_id == JX_REF_NODE) && (ref->src == NULL));
        }
      else if ((box_type != 0) && (box_type != jp2_free_4cc))
        {
          if (super_box != NULL)
            write_state->box.open(super_box,box_type);
          else
            target->open_top_box(&write_state->box,box_type,
                                 manager->simulate_write);
          if (rep_id == JX_ROI_NODE)
            { // Write an ROI description box
              assert(box_type == jp2_roi_description_4cc);
              regions->write(write_state->box);
            }
          else if (rep_id == JX_NUMLIST_NODE)
            { // Write a number list box
              assert(box_type == jp2_number_list_4cc);
              numlist->write(write_state->box);
            }
          else if (rep_id == JX_LABEL_NODE)
            { // Write a label box
              assert(box_type == jp2_label_4cc);
              write_state->box.write((kdu_byte *) label,(int) strlen(label));
            }
          else if (rep_id == JX_REF_NODE)
            {
              if (ref->src != NULL)
                { // Copy source box.
                  jp2_input_box src;
                  src.open(ref->src,ref->src_loc);
                  kdu_long body_bytes = src.get_remaining_bytes();
                  write_state->box.set_target_size(body_bytes);
                  int xfer_bytes;
                  kdu_byte buf[1024];
                  while ((xfer_bytes=src.read(buf,1024)) > 0)
                    {
                      body_bytes -= xfer_bytes;
                      write_state->box.write(buf,xfer_bytes);
                    }
                  src.close();
                  assert(body_bytes == 0);
                }
              else
                { // Application must write this box
                  if (i_param != NULL)
                    *i_param = ref->i_param;
                  if (addr_param != NULL)
                    *addr_param = ref->addr_param;
                  return &(write_state->box);
                }
            }
          else if (rep_id == JX_CROSSREF_NODE)
            {
              assert(crossref != NULL);
              write_state->box.write(crossref->box_type);
              if (crossref->box_type == 0)
                { // Set up dummy frag list with correct size for simulation
                  assert(manager->simulate_write);
                  crossref->frag_list.set_link_region(0,0);
                }
              crossref->frag_list.save_box(&(write_state->box));
            }
          else
            assert(0);
        }
      if (write_state->box.exists())
        {
          write_state->asoc_offset = 0;
          write_state->content_offset = write_state->box.get_header_length();
          write_state->node_length = write_state->box.get_box_length();
          write_state->content_length =
            write_state->node_length - write_state->content_offset;
          write_state->box.close();
        }
      write_state->active_descendant = head;
    }

  while (write_state->active_descendant != NULL)
    {
      jp2_output_box *interrupt =
        write_state->active_descendant->write(super_box,target,context,
                                              i_param,addr_param);
      if (interrupt != NULL)
        return interrupt;
      write_state->active_descendant =
        write_state->active_descendant->next_sibling;
    }

  if (write_state->asoc.exists())
    {
      write_state->asoc_offset = write_state->asoc.get_header_length();
      write_state->content_offset += write_state->asoc_offset;
      write_state->node_length = write_state->asoc.get_box_length();
      write_state->asoc.close();
    }
  flags |= JX_METANODE_WRITTEN;
  if (is_top_box)
    {
      kdu_long file_pos =
        (target->open_top_box(NULL,0,manager->simulate_write) -
         write_state->node_length);
      if (manager->simulate_write)
        discover_output_box_locations(file_pos,context);
    }
  if (!manager->simulate_write)
    {
      delete write_state;
      write_state = NULL;
    }
  return NULL; // All done
}

/*****************************************************************************/
/*                jx_metanode::discover_output_box_locations                 */
/*****************************************************************************/

kdu_long
  jx_metanode::discover_output_box_locations(kdu_long start_pos,
                                             jx_metagroup *context)
{
  if ((write_state == NULL) || (write_state->context != context))
    return start_pos;
  kdu_long entry_pos = start_pos;
  assert(write_state->active_descendant == NULL);
  if (box_type != 0)
    { // Not the root node
      kdu_long asoc_start_pos = start_pos + write_state->asoc_offset;
      start_pos += write_state->content_offset;
      jx_crossref *cref;
      for (cref=linked_from; cref != NULL; cref=cref->next_link)
        if (cref->box_type == 0)
          {
            if ((write_state->asoc_offset == 0) ||
                (cref->link_type != JPX_ALTERNATE_CHILD_LINK))
              {
                cref->box_type = box_type;
                cref->frag_list.set_link_region(start_pos,
                                                write_state->content_length);
              }
            else
              {
                cref->box_type = jp2_association_4cc;
                cref->frag_list.set_link_region(asoc_start_pos,
                                                write_state->node_length -
                                                write_state->asoc_offset);
              }
          }
      start_pos += write_state->content_length;
    }
  jx_metanode *scan;
  for (scan=head; scan != NULL; scan=scan->next_sibling)
    start_pos = scan->discover_output_box_locations(start_pos,context);
  if ((write_state->node_length > 0) &&
      (write_state->node_length != (start_pos - entry_pos)))
    assert(0);
  delete write_state;
  write_state = NULL;
  return start_pos;
}


/* ========================================================================= */
/*                              jpx_metanode                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                      jpx_metanode::get_numlist_info                       */
/*****************************************************************************/

bool
  jpx_metanode::get_numlist_info(int &num_codestreams, int &num_layers,
                                 bool &applies_to_rendered_result)
{
  if ((state == NULL) || (state->rep_id != JX_NUMLIST_NODE))
    return false;
  assert(state->flags & JX_METANODE_BOX_COMPLETE);
  num_codestreams = state->numlist->num_codestreams;
  num_layers = state->numlist->num_compositing_layers;
  applies_to_rendered_result = state->numlist->rendered_result;
  return true;
}

/*****************************************************************************/
/*                  jpx_metanode::get_numlist_codestreams                    */
/*****************************************************************************/

const int *
  jpx_metanode::get_numlist_codestreams()
{
  if ((state == NULL) || (state->rep_id != JX_NUMLIST_NODE))
    return NULL;
  return state->numlist->codestream_indices;
}

/*****************************************************************************/
/*                     jpx_metanode::get_numlist_layers                      */
/*****************************************************************************/

const int *
  jpx_metanode::get_numlist_layers()
{
  if ((state == NULL) || (state->rep_id != JX_NUMLIST_NODE))
    return NULL;
  return state->numlist->layer_indices;
}

/*****************************************************************************/
/*                  jpx_metanode::get_numlist_codestream                     */
/*****************************************************************************/

int
  jpx_metanode::get_numlist_codestream(int which)
{
  if ((state == NULL) || (state->rep_id != JX_NUMLIST_NODE) ||
      (which < 0) || (state->numlist->num_codestreams <= which))
    return -1;
  return state->numlist->codestream_indices[which];
}

/*****************************************************************************/
/*                     jpx_metanode::get_numlist_layer                       */
/*****************************************************************************/

int
  jpx_metanode::get_numlist_layer(int which)
{
  if ((state == NULL) || (state->rep_id != JX_NUMLIST_NODE) ||
      (which < 0) || (state->numlist->num_compositing_layers <= which))
    return -1;
  return state->numlist->layer_indices[which];
}

/*****************************************************************************/
/*                       jpx_metanode::test_numlist                          */
/*****************************************************************************/

bool
  jpx_metanode::test_numlist(int codestream_idx, int layer_idx,
                             bool applies_to_rendered_result)
{
  if ((state == NULL) || (state->rep_id != JX_NUMLIST_NODE))
    return false;

  int n;
  if (applies_to_rendered_result && !state->numlist->rendered_result)
    return false;
  if (codestream_idx >= 0)
    {
      for (n=0; n < state->numlist->num_codestreams; n++)
        if (state->numlist->codestream_indices[n] == codestream_idx)
          break;
      if (n == state->numlist->num_codestreams)
        return false;
    }
  if (layer_idx >= 0)
    {
      for (n=0; n < state->numlist->num_compositing_layers; n++)
        if (state->numlist->layer_indices[n] == layer_idx)
          break;
      if (n == state->numlist->num_compositing_layers)
        return false;
    }
  return true;
}

/*****************************************************************************/
/*                      jpx_metanode::get_num_regions                        */
/*****************************************************************************/

int
  jpx_metanode::get_num_regions()
{
  if ((state == NULL) || (state->rep_id != JX_ROI_NODE))
    return 0;
  assert(state->flags & JX_METANODE_BOX_COMPLETE);
  return state->regions->num_regions;
}

/*****************************************************************************/
/*                        jpx_metanode::get_regions                          */
/*****************************************************************************/

jpx_roi *jpx_metanode::get_regions()
{
  if ((state == NULL) || (state->rep_id != JX_ROI_NODE))
    return NULL;
  return state->regions->regions;
}

/*****************************************************************************/
/*                         jpx_metanode::get_region                          */
/*****************************************************************************/

jpx_roi
  jpx_metanode::get_region(int which)
{
  jpx_roi result;
  if ((state != NULL) && (state->rep_id == JX_ROI_NODE) &&
      (which >= 0) && (which < state->regions->num_regions))
    result = state->regions->regions[which];
  return result;
}

/*****************************************************************************/
/*                         jpx_metanode::get_width                           */
/*****************************************************************************/

int
  jpx_metanode::get_width()
{
  int result = 0;
  if ((state != NULL) && (state->rep_id == JX_ROI_NODE) &&
      (state->regions->num_regions > 0))
    result = state->regions->max_width;
  return result;
}
  
/*****************************************************************************/
/*                      jpx_metanode::get_bounding_box                       */
/*****************************************************************************/

kdu_dims
  jpx_metanode::get_bounding_box()
{
  kdu_dims result;
  if ((state != NULL) && (state->rep_id == JX_ROI_NODE) &&
      (state->regions->num_regions > 0))
    result = state->regions->bounding_region.region;
  return result;
}


/*****************************************************************************/
/*                        jpx_metanode::test_region                          */
/*****************************************************************************/

bool
  jpx_metanode::test_region(kdu_dims region)
{
  if ((state == NULL) || (state->rep_id != JX_ROI_NODE))
    return false;
  int n;
  for (n=0; n < state->regions->num_regions; n++)
    if (state->regions->regions[n].region.intersects(region))
      return true;
  return false;
}

/*****************************************************************************/
/*                       jpx_metanode::get_box_type                          */
/*****************************************************************************/

kdu_uint32
  jpx_metanode::get_box_type()
{
  return (state == NULL)?0:(state->box_type);
}

/*****************************************************************************/
/*                         jpx_metanode::get_label                           */
/*****************************************************************************/

const char *
  jpx_metanode::get_label()
{
  if ((state == NULL) || (state->rep_id != JX_LABEL_NODE))
    return NULL;
  return state->label;
}

/*****************************************************************************/
/*                         jpx_metanode::get_uuid                            */
/*****************************************************************************/

bool
  jpx_metanode::get_uuid(kdu_byte uuid[])
{
  if ((state == NULL) || (state->box_type != jp2_uuid_4cc) ||
      (state->rep_id != JX_REF_NODE))
    return false;
  memcpy(uuid,state->data,16);
  return true;
}

/*****************************************************************************/
/*                    jpx_metanode::get_cross_reference                      */
/*****************************************************************************/

kdu_uint32
  jpx_metanode::get_cross_reference(jpx_fragment_list &frags)
{
  if ((state == NULL) || (state->rep_id != JX_CROSSREF_NODE) ||
      (state->crossref == NULL) || (state->crossref->box_type == 0))
    return 0;
  frags = jpx_fragment_list(&(state->crossref->frag_list));
  return state->crossref->box_type;
}

/*****************************************************************************/
/*                         jpx_metanode::get_link                            */
/*****************************************************************************/

jpx_metanode
  jpx_metanode::get_link(jpx_metanode_link_type &link_type)
{
  jx_metanode *result = NULL;
  link_type = JPX_METANODE_LINK_NONE;
  if ((state != NULL) && (state->rep_id == JX_CROSSREF_NODE) &&
      (state->crossref != NULL))
    {
      result = state->crossref->link;
      link_type = state->crossref->link_type;
      if ((result != NULL) && !(result->flags & JX_METANODE_BOX_COMPLETE))
        result = NULL;
    }
  return jpx_metanode(result);
}

/*****************************************************************************/
/*                       jpx_metanode::enum_linkers                          */
/*****************************************************************************/

jpx_metanode
  jpx_metanode::enum_linkers(jpx_metanode last_linker)
{
  if ((state == NULL) || (state->linked_from == NULL))
    return jpx_metanode();
  jx_crossref *cref = state->linked_from;
  if ((last_linker.state != NULL) &&
      (last_linker.state->rep_id == JX_CROSSREF_NODE) &&
      (last_linker.state->crossref != NULL) &&
      (!(last_linker.state->flags & JX_METANODE_DELETED)) &&
      (last_linker.state->crossref->link == state))
    cref = last_linker.state->crossref->next_link;
  
  return jpx_metanode((cref==NULL)?NULL:cref->owner);
}

/*****************************************************************************/
/*                       jpx_metanode::get_existing                          */
/*****************************************************************************/

jp2_locator
  jpx_metanode::get_existing(jp2_family_src * &src)
{
  src = NULL;
  if ((state == NULL) || (state->rep_id != JX_REF_NODE))
    return jp2_locator();
  src = state->ref->src;
  return state->ref->src_loc;
}

/*****************************************************************************/
/*                        jpx_metanode::get_delayed                          */
/*****************************************************************************/

bool
  jpx_metanode::get_delayed(int &i_param, void * &addr_param)
{
  if ((state == NULL) || (state->rep_id != JX_REF_NODE) ||
      (state->ref->src != NULL))
    return false;
  i_param = state->ref->i_param;
  addr_param = state->ref->addr_param;
  return true;
}

/*****************************************************************************/
/*                     jpx_metanode::count_descendants                       */
/*****************************************************************************/

bool
  jpx_metanode::count_descendants(int &count)
{
  if (state == NULL)
    return false;
  if (!(state->flags & JX_METANODE_DESCENDANTS_KNOWN))
    {
      if (state->parent == NULL)
        { // We are at the root.  Parse as many top level boxes as possible
          jx_source *source = state->manager->source;
          while (source->parse_next_top_level_box());
          int n=0;
          jx_codestream_source *cs;
          jx_layer_source *lh;
          while ((cs = source->get_codestream(n++)) != NULL)
            cs->finish(false);
          n = 0;
          while ((lh = source->get_compositing_layer(n++)) != NULL)
            lh->finish();
        }
      else
        {
          assert((state->flags & JX_METANODE_EXISTING) &&
                 (state->read_state != NULL));
          state->finish_reading();
        }
    }
  count = state->num_descendants;
  return (state->flags & JX_METANODE_DESCENDANTS_KNOWN)?true:false;
}

/*****************************************************************************/
/*                      jpx_metanode::get_descendant                         */
/*****************************************************************************/

jpx_metanode
  jpx_metanode::get_descendant(int which)
{
  if ((state == NULL) || (which < 0) || (which >= state->num_descendants))
    return jpx_metanode(NULL);
  jx_metanode *node=state->head;
  for (; (which > 0) && (node != NULL); which--)
    node = node->next_sibling;
  if ((node != NULL) && !(node->flags & JX_METANODE_BOX_COMPLETE))
    node = NULL;
  return jpx_metanode(node);
}

/*****************************************************************************/
/*                  jpx_metanode::find_descendant_by_type                    */
/*****************************************************************************/

jpx_metanode
  jpx_metanode::find_descendant_by_type(int which, int num_box_types,
                                        const kdu_uint32 box_types[])
{
  if ((state == NULL) || (which < 0) || (which >= state->num_descendants))
    return jpx_metanode(NULL);
  jx_metanode *node=state->head;
  while (node != NULL)
    {
      bool found_match = (num_box_types == 0);
      for (int b=0; (!found_match) && (b < num_box_types); b++)
        found_match = (box_types[b] == node->box_type);
      if (found_match && (node->flags & JX_METANODE_BOX_COMPLETE))
        {
          if (which == 0)
            break;
          which--;
        }
      node = node->next_sibling;
    }
  return jpx_metanode(node);  
}

/*****************************************************************************/
/*                jpx_metanode::check_descendants_complete                   */
/*****************************************************************************/

bool
  jpx_metanode::check_descendants_complete(int num_box_types,
                                           const kdu_uint32 box_types[])
{
  if ((state == NULL) || (!(state->flags & JX_METANODE_BOX_COMPLETE)) ||
      (!(state->flags & JX_METANODE_DESCENDANTS_KNOWN)))
    return false;
  jx_metanode *child=state->head;
  for (; child != NULL; child=child->next_sibling)
    if ((!(child->flags & JX_METANODE_BOX_COMPLETE)) ||
        ((child->rep_id == JX_CROSSREF_NODE) &&
         (child->crossref->link == NULL)))
      { // See if this child has one of the box types that are of interest
        if ((num_box_types == 0) || (child->box_type == 0))
          return false;
        for (int b=0; b < num_box_types; b++)
          if (child->box_type == box_types[b])
            return false;
      }
  return true;
}

/*****************************************************************************/
/*                    jpx_metanode::get_sequence_index                       */
/*****************************************************************************/

kdu_long
  jpx_metanode::get_sequence_index()
{
  if (state == NULL)
    return 0;
  else
    return state->sequence_index;
}

/*****************************************************************************/
/*                        jpx_metanode::get_parent                           */
/*****************************************************************************/

jpx_metanode
  jpx_metanode::get_parent()
{
  return jpx_metanode((state==NULL)?NULL:state->parent);
}

/*****************************************************************************/
/*                      jpx_metanode::find_path_to                           */
/*****************************************************************************/

jpx_metanode
  jpx_metanode::find_path_to(jpx_metanode target, int descending_flags,
                             int ascending_flags, int num_exclusion_categories,
                             const kdu_uint32 exclusion_box_types[],
                             const int exclusion_flags[],
                             bool unify_groups)
{
  jx_metanode *result;
  if ((state == NULL) || (target.state == NULL))
    result = NULL;
  else if ((((ascending_flags | descending_flags) &
             (JPX_PATH_TO_FORWARD | JPX_PATH_TO_REVERSE)) == 0) &&
           !unify_groups)
    { // Perform simple search for hierarchically related nodes
      jx_metanode *scan;
      result = (state == target.state)?state:NULL;
      if (ascending_flags & JPX_PATH_TO_DIRECT)
        for (scan=state; (scan != NULL) && (result==NULL); scan=scan->parent)
          if (scan == target.state)
            result = state->parent;
      if (descending_flags & JPX_PATH_TO_DIRECT)
        for (scan=target.state;
             (scan != NULL) && (result==NULL); scan=scan->parent)
          if (scan->parent == state)
            result = scan;
    }
  else
    result = state->find_path_to(target.state,descending_flags,ascending_flags,
                                 num_exclusion_categories,
                                 exclusion_box_types,exclusion_flags,
                                 unify_groups);
  return jpx_metanode(result);
    
}

/*****************************************************************************/
/*                      jpx_metanode::change_parent                          */
/*****************************************************************************/

bool
  jpx_metanode::change_parent(jpx_metanode new_parent)
{
  if (new_parent.state == state->parent)
    return false;
  for (jx_metanode *test=new_parent.state; test != NULL; test=test->parent)
    if (test == state)
      return false;
  state->unlink_parent();
  if (state->metagroup != NULL)
    state->metagroup->unlink(state);
  new_parent.state->append_child(state);
  state->manager->link(state);
  state->flags |= JX_METANODE_ANCESTOR_CHANGED;
  state->append_to_touched_list(true);
  return true;
}


/*****************************************************************************/
/*                        jpx_metanode::add_numlist                          */
/*****************************************************************************/

jpx_metanode
  jpx_metanode::add_numlist(int num_codestreams, const int *codestream_indices,
                            int num_layers, const int *layer_indices,
                            bool applies_to_rendered_result)
{
  assert(state != NULL);
  if (state->manager->target != NULL)
    { KDU_ERROR_DEV(e,25); e <<
        KDU_TXT("Trying to add metadata to a `jpx_target' object "
        "whose `write_metadata' function as already been called.");
    }
  jx_metanode *node = state->add_numlist(num_codestreams,codestream_indices,
                                         num_layers,layer_indices,
                                         applies_to_rendered_result);

  node->flags |= (JX_METANODE_DESCENDANTS_KNOWN | JX_METANODE_IS_COMPLETE);
  state->num_completed_descendants++;
  return jpx_metanode(node);
}

/*****************************************************************************/
/*                        jpx_metanode::add_regions                          */
/*****************************************************************************/

jpx_metanode
  jpx_metanode::add_regions(int num_regions, const jpx_roi *regions)
{
  assert(state != NULL);
  if (state->manager->target != NULL)
    { KDU_ERROR_DEV(e,26); e <<
        KDU_TXT("Trying to add metadata to a `jpx_target' object "
        "whose `write_metadata' function has already been called.");
    }
  jx_metanode *node = state->add_descendant();
  node->flags |= (JX_METANODE_IS_COMPLETE | JX_METANODE_BOX_COMPLETE |
                  JX_METANODE_DESCENDANTS_KNOWN);
  state->num_completed_descendants++;
  node->box_type = jp2_roi_description_4cc;
  node->rep_id = JX_ROI_NODE;
  node->regions = new jx_regions;
  node->regions->set_num_regions(num_regions);
  node->append_to_touched_list(false);
  
  int n;
  kdu_dims rect;
  kdu_coords bound_min, bound_lim, min, lim;
  for (n=0; n < num_regions; n++)
    {
      node->regions->regions[n] = regions[n];
      node->regions->regions[n].fix_inconsistencies();
      rect = node->regions->regions[n].region;
      min = rect.pos;
      lim = min + rect.size;
      if (n == 0)
        { bound_min = min; bound_lim = lim; }
      else
        { // Grow `bound' to include `rect'
          if (min.x < bound_min.x)
            bound_min.x = min.x;
          if (min.y < bound_min.y)
            bound_min.y = min.y;
          if (lim.x > bound_lim.x)
            bound_lim.x = lim.x;
          if (lim.y > bound_lim.y)
            bound_lim.y = lim.y;
        }
    }
  node->regions->bounding_region.region.pos = bound_min;
  node->regions->bounding_region.region.size = bound_lim - bound_min;
  double wd_max = 0.1;
  for (n=0; n < num_regions; n++)
    {
      double dummy, wd=0.0;
      node->regions->regions[n].measure_span(wd,dummy);
      wd_max = (wd > wd_max)?wd:wd_max;
    }
  node->regions->max_width = (int) ceil(wd_max);
  node->manager->link(node);
  return jpx_metanode(node);
}
  
/*****************************************************************************/
/*                         jpx_metanode::add_label                           */
/*****************************************************************************/

jpx_metanode
  jpx_metanode::add_label(const char *text)
{
  jx_check_metanode_before_add_child(state);
  jx_metanode *node = state->add_descendant();
  node->flags |= (JX_METANODE_IS_COMPLETE | JX_METANODE_BOX_COMPLETE |
                  JX_METANODE_DESCENDANTS_KNOWN);
  state->num_completed_descendants++;
  node->box_type = jp2_label_4cc;
  node->label = new char[strlen(text)+1];
  strcpy(node->label,text);
  node->rep_id = JX_LABEL_NODE;
  node->manager->link(node);
  node->append_to_touched_list(false);
  return jpx_metanode(node);
}

/*****************************************************************************/
/*                      jpx_metanode::change_to_label                        */
/*****************************************************************************/

void
  jpx_metanode::change_to_label(const char *text)
{
  if (state->metagroup != NULL)
    state->metagroup->unlink(state);
  switch (state->rep_id) {
    case JX_REF_NODE:
      if (state->ref != NULL)
        { delete state->ref; state->ref = NULL; }
      break;
    case JX_NUMLIST_NODE:
      if (state->numlist != NULL)
        { delete state->numlist; state->numlist = NULL; }
      break;
    case JX_ROI_NODE:
      if (state->regions != NULL)
        { delete state->regions; state->regions = NULL; }
      break;
    case JX_LABEL_NODE:
      if (state->label != NULL)
        { delete state->label; state->label = NULL; }
      break;
    case JX_CROSSREF_NODE:
      if (state->crossref != NULL)
        { delete state->crossref; state->crossref = NULL; }
      break;
    default:
      assert(0);
      break;
  };
  state->box_type = jp2_label_4cc;
  state->label = new char[strlen(text)+1];
  strcpy(state->label,text);  
  state->rep_id = JX_LABEL_NODE;
  if ((state->flags & JX_METANODE_EXISTING) && (state->read_state != NULL))
    { delete state->read_state; state->read_state = NULL; }
  state->flags &= ~(JX_METANODE_EXISTING | JX_METANODE_WRITTEN);
  state->flags |= (JX_METANODE_IS_COMPLETE | JX_METANODE_BOX_COMPLETE |
                   JX_METANODE_DESCENDANTS_KNOWN |
                   JX_METANODE_CONTENTS_CHANGED);
  state->manager->link(state);
  state->append_to_touched_list(true);
  jx_crossref *cref;
  for (cref=state->linked_from; cref != NULL; cref=cref->next_link)
    cref->owner->append_to_touched_list(true);
}

/*****************************************************************************/
/*                        jpx_metanode::add_delayed                          */
/*****************************************************************************/

jpx_metanode
  jpx_metanode::add_delayed(kdu_uint32 box_type, int i_param, void *addr_param)
{
  jx_check_metanode_before_add_child(state);
  jx_metanode *node = state->add_descendant();
  node->flags |= (JX_METANODE_IS_COMPLETE | JX_METANODE_BOX_COMPLETE |
                  JX_METANODE_DESCENDANTS_KNOWN |
                  JX_METANODE_CONTENTS_CHANGED);
  state->num_completed_descendants++;
  node->box_type = box_type;
  node->rep_id = JX_REF_NODE;
  node->ref = new jx_metaref;
  node->ref->i_param = i_param;
  node->ref->addr_param = addr_param;
  node->manager->link(node);
  node->append_to_touched_list(false);
  return jpx_metanode(node);
}

/*****************************************************************************/
/*                     jpx_metanode::change_to_delayed                       */
/*****************************************************************************/

void
  jpx_metanode::change_to_delayed(kdu_uint32 box_type, int i_param,
                                  void *addr_param)
{
  if (state->metagroup != NULL)
    state->metagroup->unlink(state);
  switch (state->rep_id) {
    case JX_REF_NODE:
      if (state->ref != NULL)
        { delete state->ref; state->ref = NULL; }
      break;
    case JX_NUMLIST_NODE:
      if (state->numlist != NULL)
        { delete state->numlist; state->numlist = NULL; }
      break;
    case JX_ROI_NODE:
      if (state->regions != NULL)
        { delete state->regions; state->regions = NULL; }
      break;
    case JX_LABEL_NODE:
      if (state->label != NULL)
        { delete state->label; state->label = NULL; }
      break;
    case JX_CROSSREF_NODE:
      if (state->crossref != NULL)
        { delete state->crossref; state->crossref = NULL; }
      break;
    default:
      assert(0);
      break;
  };
  state->box_type = box_type;
  state->rep_id = JX_REF_NODE;
  state->ref = new jx_metaref;
  state->ref->i_param = i_param;
  state->ref->addr_param = addr_param;  
  if ((state->flags & JX_METANODE_EXISTING) && (state->read_state != NULL))
    { delete state->read_state; state->read_state = NULL; }
  state->flags &= ~(JX_METANODE_EXISTING | JX_METANODE_WRITTEN);
  state->flags |= (JX_METANODE_IS_COMPLETE | JX_METANODE_BOX_COMPLETE |
                   JX_METANODE_DESCENDANTS_KNOWN |
                   JX_METANODE_CONTENTS_CHANGED);
  
  state->manager->link(state);
  state->append_to_touched_list(true);
  jx_crossref *cref;
  for (cref=state->linked_from; cref != NULL; cref=cref->next_link)
    cref->owner->append_to_touched_list(true);
}

/*****************************************************************************/
/*                         jpx_metanode::add_link                            */
/*****************************************************************************/

jpx_metanode
  jpx_metanode::add_link(jpx_metanode target, jpx_metanode_link_type link_type,
                         bool avoid_duplicates)
{
  jx_check_metanode_before_add_child(state);
  if ((target.state == NULL) || (target.state->flags & JX_METANODE_DELETED))
    { KDU_ERROR_DEV(e,0x21040901); e <<
      KDU_TXT("Trying to add a metadata link to a metadata node which "
              "appears to have been deleted.");
    }
  if ((target.state->rep_id == JX_CROSSREF_NODE) &&
      ((target.state->crossref->link_type != JPX_GROUPING_LINK) ||
       (link_type != JPX_ALTERNATE_CHILD_LINK)))
    { KDU_ERROR(e,0x01060901); e <<
      KDU_TXT("Trying to add a metadata link to a metadata node which itself "
              "is a link.  This is legal only if the target node has "
              "the \"Grouping\" link-type (then the cross-reference can "
              "refer to its containing `asoc' box) and then only if the link "
              "you are trying to add has the \"Alternate Child\" link "
              "type (otherwise the cross-reference cannot refer to the "
              "`asoc' box).");
    }
  
  jx_metanode *node = NULL;
  if (avoid_duplicates)
    for (node=state->head; node != NULL; node=node->next_sibling)
      if ((node->rep_id == JX_CROSSREF_NODE) && (node->crossref != NULL) &&
          (node->crossref->link == target.state) &&
          (node->crossref->link_type == link_type))
        return jpx_metanode(node);
  
  node = state->add_descendant();
  node->flags |= (JX_METANODE_IS_COMPLETE | JX_METANODE_BOX_COMPLETE |
                  JX_METANODE_DESCENDANTS_KNOWN |
                  JX_METANODE_CONTENTS_CHANGED);
  state->num_completed_descendants++;
  node->box_type = jp2_cross_reference_4cc;
  node->rep_id = JX_CROSSREF_NODE;
  node->crossref = new jx_crossref;
  node->crossref->owner = node;
  node->crossref->link = target.state;
  node->crossref->link_type = link_type;
  if (target.state->linked_from == NULL)
    target.state->linked_from = node->crossref;
  else
    node->crossref->append_to_list(target.state->linked_from);
  node->manager->link(node);
  node->append_to_touched_list(false);
  node->manager->have_link_nodes = true;
  return jpx_metanode(node);
}

/*****************************************************************************/
/*                      jpx_metanode::change_to_link                         */
/*****************************************************************************/

void
  jpx_metanode::change_to_link(jpx_metanode target,
                               jpx_metanode_link_type link_type)
{
  if ((target.state == NULL) || (target.state->flags & JX_METANODE_DELETED))
    { KDU_ERROR_DEV(e,0x01060902); e <<
      KDU_TXT("Trying to convert a metadata node into a link which points to "
              "another metadata node that appears to have been deleted.");
    }
  if (state->linked_from != NULL)
    { // See if the linkers are compatible
      bool is_compatible = (link_type == JPX_GROUPING_LINK);
      jx_crossref *cref;
      for (cref=state->linked_from; is_compatible && (cref != NULL);
           cref=cref->next_link)
        if (cref->link_type != JPX_ALTERNATE_CHILD_LINK)
          is_compatible = false;
      if (!is_compatible)
        { KDU_ERROR(e,0x01050901); e <<
          KDU_TXT("Attempting to convert a `jpx_metanode' entry in the "
                  "metadata hierarchy into a link, whereas there are other "
                  "nodes which are links and point to this node.  This is "
                  "legal only if the converted node is to have the "
                  "\"Grouping\" link-type and each of the existing linking "
                  "nodes has \"Alternate-child\" link-type -- otherwise the "
                  "representation within the JPX file format would require a "
                  "cross-reference box to point to another cross-reference "
                  "box.");
        }
    }
  if (target.state == state)
    { KDU_ERROR(e,0x01060903); e <<
      KDU_TXT("Attempting to convert a `jpx_metanode' entry in the "
              "metadata hierarchy into a link which points to itself.  "
              "This cannot be achieved legally, since a link node can "
              "be referenced by another link node only if the referenced "
              "node has the \"Grouping\" link-type and the referring "
              "node has the \"Alternate-child\" link-type.");
    }
  else if ((target.state->rep_id == JX_CROSSREF_NODE) &&
           ((target.state->crossref->link_type != JPX_GROUPING_LINK) ||
            (link_type != JPX_ALTERNATE_CHILD_LINK)))
    { KDU_ERROR(e,0x01050902); e <<
      KDU_TXT("Attempting to convert a `jpx_metanode' entry in the metadata "
              "hierarchy into a link to another node which is itself a link "
              "or other form of cross-reference box.  This is legal only if "
              "the target node has the \"Grouping\" link type and node being "
              "converted is being assigned an \"Alternate-child\" link "
              "type -- otherwise, the representation within the JPX file "
              "format would require a cross-reference box "
              "to point to another cross-reference box.");
      }
  if ((link_type != JPX_GROUPING_LINK) && (state->num_descendants > 0))
    { KDU_ERROR(e,0x02050903); e <<
      KDU_TXT("Attempting to convert a `jpx_metanode' entry in the metadata "
              "hierarchy into a non-grouping link, where the node being "
              "converted already has descendants.  Link nodes which have "
              "descendants are automatically interpreted as grouping links.");
    }
  if (state->metagroup != NULL)
    state->metagroup->unlink(state);
  switch (state->rep_id) {
    case JX_REF_NODE:
      if (state->ref != NULL)
        { delete state->ref; state->ref = NULL; }
      break;
    case JX_NUMLIST_NODE:
      if (state->numlist != NULL)
        { delete state->numlist; state->numlist = NULL; }
      break;
    case JX_ROI_NODE:
      if (state->regions != NULL)
        { delete state->regions; state->regions = NULL; }
      break;
    case JX_LABEL_NODE:
      if (state->label != NULL)
        { delete state->label; state->label = NULL; }
      break;
    case JX_CROSSREF_NODE:
      if (state->crossref != NULL)
        { delete state->crossref; state->crossref = NULL; }
      break;
    default:
      assert(0);
      break;
  };
  
  state->box_type = jp2_cross_reference_4cc;
  state->rep_id = JX_CROSSREF_NODE;
  state->crossref = new jx_crossref;
  state->crossref->owner = state;
  state->crossref->link = target.state;
  state->crossref->link_type = link_type;
  if (target.state->linked_from == NULL)
    target.state->linked_from = state->crossref;
  else
    state->crossref->append_to_list(target.state->linked_from);
  if ((state->flags & JX_METANODE_EXISTING) && (state->read_state != NULL))
    { delete state->read_state; state->read_state = NULL; }
  state->flags &= ~(JX_METANODE_EXISTING | JX_METANODE_WRITTEN);
  state->flags |= (JX_METANODE_IS_COMPLETE | JX_METANODE_BOX_COMPLETE |
                   JX_METANODE_DESCENDANTS_KNOWN |
                   JX_METANODE_CONTENTS_CHANGED);
  state->manager->link(state);
  state->append_to_touched_list(true);
  state->manager->have_link_nodes = true;
}

/*****************************************************************************/
/*                          jpx_metanode::add_copy                           */
/*****************************************************************************/

jpx_metanode
  jpx_metanode::add_copy(jpx_metanode src, bool recursive,
                         bool link_to_internal_copies)
{
  jx_check_metanode_before_add_child(state);
  assert((state != NULL) && (src.state != NULL));

  jpx_metanode result;
  jx_metaloc_manager &metaloc_manager = state->manager->metaloc_manager; 
  bool different_manager = (src.state->manager != state->manager);
  bool link_to_copies = link_to_internal_copies || different_manager;

  if (src.state->rep_id == JX_NUMLIST_NODE)
    result = add_numlist(src.state->numlist->num_codestreams,
                         src.state->numlist->codestream_indices,
                         src.state->numlist->num_compositing_layers,
                         src.state->numlist->layer_indices,
                         src.state->numlist->rendered_result);
  else if (src.state->rep_id == JX_ROI_NODE)
    result = add_regions(src.state->regions->num_regions,
                         src.state->regions->regions);
  else if (src.state->rep_id == JX_LABEL_NODE)
    result = add_label(src.state->label);
  else if (src.state->rep_id == JX_REF_NODE)
    {
      jx_metanode *node = state->add_descendant();
      node->flags |= (JX_METANODE_IS_COMPLETE | JX_METANODE_BOX_COMPLETE |
                      JX_METANODE_DESCENDANTS_KNOWN);
      state->num_completed_descendants++;
      node->box_type = src.state->box_type;
      node->rep_id = JX_REF_NODE;
      node->ref = new jx_metaref;
      *(node->ref) = *(src.state->ref);
      node->manager->link(node);
      node->append_to_touched_list(false);
      result = jpx_metanode(node);
    }
  else if (src.state->rep_id == JX_CROSSREF_NODE)
    {
      if (src.state->crossref == NULL)
        return result;
      jx_metanode *src_link = src.state->crossref->link;
      if (src_link == NULL)
        return result; // Do not copy cross-reference nodes except known links
      jx_metanode *node = state->add_descendant();
      node->flags |= (JX_METANODE_IS_COMPLETE | JX_METANODE_BOX_COMPLETE |
                      JX_METANODE_DESCENDANTS_KNOWN);
      state->num_completed_descendants++;
      node->box_type = src.state->box_type;
      node->rep_id = JX_CROSSREF_NODE;
      node->crossref = new jx_crossref;
      node->crossref->owner = node;
      node->crossref->link_type = src.state->crossref->link_type;
      node->manager->link(node);
      node->append_to_touched_list(false);
      if (!link_to_copies)
        {
          node->crossref->link = src_link;
          node->crossref->append_to_list(src_link->linked_from);
        }
      else
        {
          jx_metaloc *metaloc =
            metaloc_manager.get_copy_locator(src_link,true);
          if (metaloc->target == NULL)
            { // Temporarily make `metaloc' point to ourselves
              node->crossref->metaloc = metaloc;
              metaloc->target = node;
            }
          else if ((metaloc->target->rep_id == JX_CROSSREF_NODE) &&
                   (metaloc->target->crossref != NULL) &&
                   (metaloc->target->crossref->metaloc == metaloc))
            {
              node->crossref->append_to_list(metaloc->target->crossref);
              node->crossref->metaloc = metaloc;
            }
          else if (metaloc->target->flags & JX_METANODE_DELETED)
            { // We will not be able to complete the link
              node->safe_delete();
              return result; // This condition is extremely unlikely, since
                             // everything is normally copied at once.
            }
          else
            {
              if (metaloc->target->linked_from == NULL)
                metaloc->target->linked_from = node->crossref;
              else
                node->crossref->append_to_list(metaloc->target->linked_from);
              node->crossref->metaloc = metaloc; // So `link_found' works
              node->crossref->link_found();
            }
        }
      result = jpx_metanode(node);
    }
  else if (src.state->rep_id == JX_NULL_NODE)
    return result; // Return empty interface
  else
    assert(0);
  
  if (link_to_copies && (src.state->linked_from != NULL))
    { // Source object has links so we should set ourselves up to be able to
      // accept links -- perhaps there are even some waiting.  To do this, we
      // associate a `metaloc' object with the address of the node from which
      // we were copied.
      jx_crossref *scan;
      jx_metaloc *metaloc = metaloc_manager.get_copy_locator(src.state,true);
      if ((metaloc->target != NULL) &&
          (metaloc->target->rep_id == JX_CROSSREF_NODE) &&
          (metaloc->target->crossref->metaloc == metaloc))
        result.state->linked_from = metaloc->target->crossref;
      metaloc->target = result.state;
      for (scan=result.state->linked_from; scan != NULL; scan=scan->next_link)
        scan->link_found();
    }
  
  if (recursive)
    {
      src.state->finish_reading();      
      for (jx_metanode *scan=src.state->head;
           scan != NULL; scan=scan->next_sibling)
        {
          scan->finish_reading();
          if (!(scan->flags & JX_METANODE_BOX_COMPLETE))
            continue;
          result.add_copy(jpx_metanode(scan),true,link_to_internal_copies);
        }
    }
  return result;
}

/*****************************************************************************/
/*                        jpx_metanode::delete_node                          */
/*****************************************************************************/

void
  jpx_metanode::delete_node()
{
  assert(state != NULL);

  if ((state->parent != NULL) && (state->parent->rep_id == JX_NUMLIST_NODE) &&
      (state->parent->head == state) && (state->parent->tail == state) &&
      (state->metagroup != NULL) && (state->metagroup->roigroup != NULL))
    { // We are deleting an ROI description object whose parent serves
      // only to associate it with a number list.  Kill the parent too.
      state->parent->safe_delete();
          // Does all the unlinking and puts on deleted list
    }
  else
    state->safe_delete(); // Does all the unlinking and puts on deleted list
  state = NULL;
}

/*****************************************************************************/
/*                        jpx_metanode::is_changed                           */
/*****************************************************************************/

bool
  jpx_metanode::is_changed()
{
  return ((state != NULL) &&
          ((state->flags & JX_METANODE_CONTENTS_CHANGED) ||
           ((state->rep_id == JX_CROSSREF_NODE) &&
            (state->crossref != NULL) && (state->crossref->link != NULL) &&
            (state->crossref->link->flags & JX_METANODE_CONTENTS_CHANGED))));
}

/*****************************************************************************/
/*                     jpx_metanode::ancestor_changed                        */
/*****************************************************************************/

bool
  jpx_metanode::ancestor_changed()
{
  return ((state != NULL) && (state->flags & JX_METANODE_ANCESTOR_CHANGED));  
}

/*****************************************************************************/
/*                        jpx_metanode::is_deleted                           */
/*****************************************************************************/

bool
  jpx_metanode::is_deleted()
{
  return ((state == NULL) || (state->flags & JX_METANODE_DELETED));  
}

/*****************************************************************************/
/*                          jpx_metanode::touch                              */
/*****************************************************************************/

void
  jpx_metanode::touch()
{
  if (state != NULL)
    state->append_to_touched_list(true);
}

/*****************************************************************************/
/*                       jpx_metanode::set_state_ref                         */
/*****************************************************************************/

void
  jpx_metanode::set_state_ref(void *ref)
{
  if (state != NULL)
    state->app_state_ref = ref;
}

/*****************************************************************************/
/*                       jpx_metanode::get_state_ref                         */
/*****************************************************************************/

void *
  jpx_metanode::get_state_ref()
{
  return (state==NULL)?NULL:(state->app_state_ref);
}

/*****************************************************************************/
/*                       jpx_metanode::generate_metareq                      */
/*****************************************************************************/

int
  jpx_metanode::generate_metareq(kdu_window *client_window,
                                 int num_box_types,
                                 const kdu_uint32 *box_types,
                                 int num_descend_types,
                                 const kdu_uint32 *descend_types,
                                 bool priority)
{
  if ((state == NULL) ||
      ((state->box_type != 0) && !(state->flags & JX_METANODE_EXISTING)))
    return 0;
  if ((state->flags & JX_METANODE_DESCENDANTS_KNOWN) &&
      (state->num_descendants == state->num_completed_descendants))
    return 0; // All data under this node is already available

  // See if we can obtain our databin id and nesting level directly
  kdu_long bin_id=-1; // Databin containing node's asoc box contents
  int nesting_level=0; // Nesting level of asoc contents within container
  if (state->read_state != NULL)
    { // Databin and nesting level of `box' header are those of `asoc' contents
      bin_id = state->read_state->box_databin_id;
      nesting_level = state->read_state->box_nesting_level;
    }
  
  // Scan children to see what requests are required and, if necessary,
  // recover `bin_id' and `nesting_level' from an incomplete child.
  jx_metanode *child;
  bool have_missing_children =
    !(state->flags & JX_METANODE_DESCENDANTS_KNOWN);
  bool need_metareq = have_missing_children;
  bool have_unknown_child_type = false;
  kdu_uint32 needed_box_type=0; // Becomes FFFFFFFF if more than one
  for (child=state->head; child != NULL; child=child->next_sibling)
    {
      if ((bin_id < 0) && (child->read_state != NULL))
        { // Can recover `bin_id' and `nesting_level' from this child
          if ((bin_id = child->read_state->asoc_databin_id) >= 0)
            nesting_level = child->read_state->asoc_nesting_level;
          else if ((bin_id = child->read_state->box_databin_id) >= 0)
            nesting_level = child->read_state->box_nesting_level;
        }
      if ((child->flags & JX_METANODE_BOX_COMPLETE) &&
          ((child->rep_id != JX_CROSSREF_NODE) ||
           (child->crossref->link != NULL)))
        continue; // Now we have it
      if (child->box_type == 0)
        {
          have_unknown_child_type = true;
          need_metareq = true;
        }
      else
        {
          if (num_box_types > 0)
            { // See if this is a type we actually need
              int b;
              for (b=0; b < num_box_types; b++)
                if (box_types[b] == child->box_type)
                  break;
              if (b == num_box_types)
                continue; // Don't need this box type
            }
          need_metareq = true; // Have an incomplete child which we need
          if (needed_box_type == 0)
            needed_box_type = child->box_type;
          else if (needed_box_type != child->box_type)
            needed_box_type = 0xFFFFFFFF;
        }
    }
  
  int num_metareqs = 0;
  if (need_metareq && (bin_id >= 0))
    {
      if (have_missing_children)
        { // Ask for the headers of all immediate children
          client_window->add_metareq(0,KDU_MRQ_ALL,priority,
                                     0,false,bin_id,nesting_level);
          num_metareqs++;
        }
      if (have_unknown_child_type)
        { // Ask for the first 16 bytes of all boxes found at the top level
          // of the node's `asoc' box contents.  This ensures that we get
          // all headers of the initial sub-box of each asoc box.
          client_window->add_metareq(jp2_association_4cc,KDU_MRQ_ALL,priority,
                                     16,false,bin_id,nesting_level);
          num_metareqs++;
        }
      if (needed_box_type != 0)
        { // Otherwise, we don't have any specific box types to request here
          if (needed_box_type != 0xFFFFFFFF)
            { // Just ask for this one box type
              client_window->add_metareq(needed_box_type,KDU_MRQ_ALL,priority,
                                         INT_MAX,false,bin_id,nesting_level+1);
              num_metareqs++;
            }
          else if (num_box_types == 0)
            { // Have to ask for all possible box types
              client_window->add_metareq(0,KDU_MRQ_ALL,priority,
                                         INT_MAX,false,bin_id,nesting_level+1);
              num_metareqs++;
            }
          else
            for (int b=0; b < num_box_types; b++, num_metareqs++)
              client_window->add_metareq(box_types[b],KDU_MRQ_ALL,priority,
                                         INT_MAX,false,bin_id,nesting_level+1);
        }
    }
  
  if (num_descend_types == 0)
    return num_metareqs;
  for (child=state->head; child != NULL; child=child->next_sibling)
    {
      if (child->box_type == 0)
        continue;
      if ((child->flags & JX_METANODE_BOX_COMPLETE) &&
          (child->flags & JX_METANODE_DESCENDANTS_KNOWN) &&
          (child->num_descendants == child->num_completed_descendants))
        continue;
      for (int b=0; b < num_descend_types; b++)
        if (child->box_type == descend_types[b])
          {
            jpx_metanode child_metanode(child);
            num_metareqs +=
              child_metanode.generate_metareq(client_window,num_box_types,
                                              box_types,num_descend_types,
                                              descend_types,priority);
          }
    }  

  return num_metareqs;
}

/*****************************************************************************/
/*                   jpx_metanode::generate_link_metareq                     */
/*****************************************************************************/

int
  jpx_metanode::generate_link_metareq(kdu_window *client_window,
                                      int num_box_types,
                                      const kdu_uint32 *box_types,
                                      int num_descend_types,
                                      const kdu_uint32 *descend_types,
                                      bool priority)
{
  if ((state == NULL) || (state->rep_id != JX_CROSSREF_NODE) ||
      !(state->flags & JX_METANODE_EXISTING))
    return 0;
  jx_crossref *cref = state->crossref;
  if (cref == NULL)
    return 0;
  jx_metanode *tgt = cref->link;
  if (tgt == NULL)
    return 0;
  
  bool tgt_box_of_interest = true;
  bool descend_through_tgt_box = false;
  if (tgt->box_type != 0)
    {
      int b;
      if (num_box_types > 0)
        for (tgt_box_of_interest=false, b=0; b < num_box_types; b++)
          if (box_types[b] == tgt->box_type)
            { tgt_box_of_interest = true; break; }
      if (tgt_box_of_interest)
        for (b=0; b < num_descend_types; b++)
          if (box_types[b] == tgt->box_type)
            descend_through_tgt_box = true;
    }
  if (!tgt_box_of_interest)
    return 0;
  
  int num_metareqs = 0;
  if ((tgt->read_state != NULL) && !(tgt->flags & JX_METANODE_BOX_COMPLETE))
    { // Need to generate requests for the target box itself
      if (tgt->box_type != 0)
        client_window->add_metareq(tgt->box_type,KDU_MRQ_ALL,priority,
                                   INT_MAX,false,
                                   tgt->read_state->box_databin_id,
                                   tgt->read_state->box_nesting_level);
      else if (cref->box_type != jp2_association_4cc)
        client_window->add_metareq(cref->box_type,KDU_MRQ_ALL,priority,
                                   INT_MAX,false,
                                   tgt->read_state->box_databin_id,
                                   tgt->read_state->box_nesting_level);
      else if (tgt->read_state->box_databin_id !=
               tgt->read_state->asoc_databin_id)
        client_window->add_metareq(jp2_association_4cc,KDU_MRQ_ALL,priority,
                                   16,false, // Enough for first sub-box header
                                   tgt->read_state->asoc_databin_id,
                                   tgt->read_state->asoc_nesting_level);
      else
        client_window->add_metareq(0,KDU_MRQ_ALL,priority,
                                   INT_MAX,false,
                                   tgt->read_state->box_databin_id,
                                   tgt->read_state->box_nesting_level);
      num_metareqs = 1;
    }

  if (descend_through_tgt_box)
    {
      jpx_metanode tgt_metanode(tgt);
      num_metareqs +=
        tgt_metanode.generate_metareq(client_window,num_box_types,box_types,
                                      num_descend_types,descend_types,
                                      priority);
    }
  return num_metareqs;
}


/* ========================================================================= */
/*                              jx_metagroup                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                           jx_metagroup::link                              */
/*****************************************************************************/

void
  jx_metagroup::link(jx_metanode *node)
{
  node->metagroup = this;
  node->prev_link = tail;
  node->next_link = NULL;
  if (tail == NULL)
    head = tail = node;
  else
    tail = tail->next_link = node;
}

/*****************************************************************************/
/*                          jx_metagroup::unlink                             */
/*****************************************************************************/

void
  jx_metagroup::unlink(jx_metanode *node)
{
  assert(node->metagroup == this);
  if (node->prev_link == NULL)
    {
      assert(node == head);
      head = node->next_link;
    }
  else
    node->prev_link->next_link = node->next_link;
  if (node->next_link == NULL)
    {
      assert(node == tail);
      tail = node->prev_link;
    }
  else
    node->next_link->prev_link = node->prev_link;
  node->metagroup = NULL;
  node->next_link = node->prev_link = NULL;

  jx_roigroup *roig = roigroup;
  if ((head == NULL) && (roig != NULL))
    {
      assert(roig->level == 0);
      int offset = (int)(this - roig->metagroups);
      assert((offset >= 0) && (offset < (JX_ROIGROUP_SIZE*JX_ROIGROUP_SIZE)));
      kdu_coords loc;
      loc.y = offset / JX_ROIGROUP_SIZE;
      loc.x = offset - loc.y * JX_ROIGROUP_SIZE;
      roig->delete_child(loc);
    }
}

/*****************************************************************************/
/*                     jx_metagroup::mark_for_writing                        */
/*****************************************************************************/

bool
  jx_metagroup::mark_for_writing()
{
  jx_metanode *scan, *parent;
  bool marked_something = false;
  for (scan=head; scan != NULL; scan=scan->next_link)
    {
      if (scan->mark_for_writing(this))
        { // Mark all parents on which this one depends for writing
          marked_something = true;
          for (parent=scan->parent; parent != NULL; parent=parent->parent)
            {
              if (parent->write_state == NULL)
                {
                  parent->write_state = new jx_metawrite;
                  parent->flags &= ~(JX_METANODE_WRITTEN);
                }
              assert(!(parent->flags & JX_METANODE_WRITTEN));
              if (parent->write_state->context == this)
                break; // Already taken care of this one
              parent->write_state->context = this;
              assert((parent->write_state->active_descendant == NULL) &&
                     (!parent->write_state->asoc) &&
                     (!parent->write_state->box));
            }
        }
    }
  return marked_something;
}


/* ========================================================================= */
/*                               jx_roigroup                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                        jx_roigroup::~jx_roigroup                          */
/*****************************************************************************/

jx_roigroup::~jx_roigroup()
{
  if (level == 0)
    return;
  jx_roigroup **scan = sub_groups;
  for (int n=JX_ROIGROUP_SIZE*JX_ROIGROUP_SIZE; n > 0; n--, scan++)
    if (*scan != NULL)
      delete *scan;
}

/*****************************************************************************/
/*                        jx_roigroup::delete_child                          */
/*****************************************************************************/

void
  jx_roigroup::delete_child(kdu_coords loc)
{
  assert((loc.x >= 0) && (loc.x < JX_ROIGROUP_SIZE) &&
         (loc.y >= 0) && (loc.y < JX_ROIGROUP_SIZE));
  int n, offset = loc.x + loc.y*JX_ROIGROUP_SIZE;
  bool any_left = false;
  if (level == 0)
    {
      assert(metagroups[offset].head == NULL);
      for (n=0; n < (JX_ROIGROUP_SIZE*JX_ROIGROUP_SIZE); n++)
        if (metagroups[n].head != NULL)
          { any_left = true; break; }
    }
  else
    {
      assert(sub_groups[offset] != NULL);
      delete sub_groups[offset];
      sub_groups[offset] = NULL;
      for (n=0; n < (JX_ROIGROUP_SIZE*JX_ROIGROUP_SIZE); n++)
        if (sub_groups[n] != NULL)
          { any_left = true; break; }
    }
  if (any_left)
    return;

  if (parent == NULL)
    {
      assert(this == manager->roi_scales[scale_idx]);
      manager->roi_scales[scale_idx] = NULL;
      delete this;
    }
  else
    {
      loc = group_dims.pos - parent->group_dims.pos;
      loc.x = loc.x / parent->elt_size.x;
      loc.y = loc.y / parent->elt_size.y;
      assert((loc.x >= 0) && (loc.x < JX_ROIGROUP_SIZE) &&
             (loc.y >= 0) && (loc.y < JX_ROIGROUP_SIZE));
      assert(this == parent->sub_groups[loc.x+loc.y*JX_ROIGROUP_SIZE]);
      parent->delete_child(loc);
    }
}


/* ========================================================================= */
/*                            jx_metaloc_manager                             */
/* ========================================================================= */

/*****************************************************************************/
/*                  jx_metaloc_manager::jx_metaloc_manager                   */
/*****************************************************************************/

jx_metaloc_manager::jx_metaloc_manager()
{
  locator_heap = NULL;
  block_heap = NULL;
  root = allocate_block(); // Allocates an empty block of references
}
  
/*****************************************************************************/
/*                  jx_metaloc_manager::~jx_metaloc_manager                  */
/*****************************************************************************/

jx_metaloc_manager::~jx_metaloc_manager()
{
  jx_metaloc_alloc *elt;
  while ((elt=locator_heap) != NULL)
    {
      locator_heap = elt->next;
      delete elt;
    }
  jx_metaloc_block_alloc *blk;
  while ((blk=block_heap) != NULL)
    {
      block_heap = blk->next;
      delete blk;
    }
}

/*****************************************************************************/
/*                      jx_metaloc_manager::get_locator                      */
/*****************************************************************************/

jx_metaloc *
  jx_metaloc_manager::get_locator(kdu_long pos, bool create_if_necessary)
{
  jx_metaloc_block *container = root;
  int idx;
  
  jx_metaloc *elt = container;
  while ((elt != NULL) && (elt == container))
    {
      elt = NULL;
      for (idx=container->active_elts-1; idx >= 0; idx--)
        if (container->elts[idx]->loc <= pos)
          { elt = container->elts[idx];  break; }
      if ((elt != NULL) && elt->is_block())
        container = (jx_metaloc_block *) elt; // Descend into sub-block
    }
  if ((elt != NULL) && (elt->loc == pos))
    return elt; // Found an existing match

  if (!create_if_necessary)
    return NULL;
  
  // If we get here, we have to create a new element
  // Ideally, the element goes right after location `idx' within
  // `container'; if `idx' is -ve, we need to position it before the first
  // element.  We can shuffle elements within `container' around to make this
  // possible, unless `container' is full. In that case, we want to adjust
  // things while keeping the tree as balanced as possible.  To do that, we
  // insert a new container at the same level and redistribute the branches.
  // Inserting a new container may require recursive application of the
  // procedure all the way up to the root of the tree.  These operations are
  // all accomplished by the recursive function `insert_into_metaloc_block'.
  assert(container != NULL);
  elt = allocate_locator();
  elt->loc = pos;
  insert_into_metaloc_block(container,elt,idx);
  return elt;
}

/*****************************************************************************/
/*              jx_metaloc_manager::insert_into_metaloc_block                */
/*****************************************************************************/

void
  jx_metaloc_manager::insert_into_metaloc_block(jx_metaloc_block *container,
                                                jx_metaloc *elt, int idx)
{
  assert((idx >= -1) && (idx < container->active_elts));
  int n;
  if (container->active_elts < JX_METALOC_BLOCK_DIM)
    {
      for (n=container->active_elts-1; n > idx; n--)
        container->elts[n+1] = container->elts[n]; // Shuffle existing elements
      container->active_elts++;
      container->elts[idx+1] = elt;
      if (idx == -1)
        container->loc = elt->loc; // Container location comes from first elt
      if (elt->is_block())
        ((jx_metaloc_block *) elt)->parent = container;
    }
  else
    { // Container is full, need to insert a new container at the same level
      jx_metaloc_block *new_container = allocate_block();
      if (idx == (container->active_elts-1))
        { // Don't bother redistributing elements between `container' and
          // `new_container'; we are probably just discovering elements in
          // linear order here, so it is probably wasteful to plan for fuure
          // insertions at this point.
          new_container->elts[0] = elt;
          new_container->active_elts = 1;
          new_container->loc = elt->loc;
          if (elt->is_block())
            ((jx_metaloc_block *) elt)->parent = new_container;
        }
      else
        {
          new_container->active_elts = (container->active_elts >> 1);
          container->active_elts -= new_container->active_elts;
          for (n=0; n < new_container->active_elts; n++)
            {
              jx_metaloc *moving_elt=container->elts[container->active_elts+n];
              new_container->elts[n] = moving_elt;
              if (moving_elt->is_block())
                ((jx_metaloc_block *) moving_elt)->parent = new_container;
            }
          assert((container->active_elts > 0) &&
                 (new_container->active_elts > 0));
          new_container->loc = new_container->elts[0]->loc;
          if (idx < container->active_elts)
            insert_into_metaloc_block(container,elt,idx);
          else
            insert_into_metaloc_block(new_container,elt,
                                      idx-container->active_elts);
        }

      jx_metaloc_block *parent = container->parent;
      if (parent == NULL)
        { // Create new root node
          assert(container == root);
          root = allocate_block();
          root->active_elts = 2;
          root->elts[0] = container;
          root->elts[1] = new_container;
          root->loc = container->loc;
          container->parent = root;
          new_container->parent = root;
        }
      else
        {
          for (n=0; n < parent->active_elts; n++)
            if (parent->elts[n] == container)
              { // This condition should be matched by exactly one entry; if
                // not, nothing disastrous happens, we just don't insert the
                // new container and so we lose some references, but there
                // will be no memory leaks.
                insert_into_metaloc_block(parent,new_container,n);
                break;
              }
        }
    }
}


/* ========================================================================= */
/*                             jx_meta_manager                               */
/* ========================================================================= */

/*****************************************************************************/
/*                      jx_meta_manager::jx_meta_manager                     */
/*****************************************************************************/

jx_meta_manager::jx_meta_manager()
{
  ultimate_src = NULL;
  source = NULL;
  target = NULL;
  tree = NULL;
  next_sequence_index = 0;
  unassociated_nodes.init();
  numlist_nodes.init();
  for (int n=0; n < 32; n++)
    roi_scales[n] = NULL;
  last_added_node = NULL;
  active_metagroup = NULL;
  active_metagroup_out_pos = 0;
  num_filter_box_types = max_filter_box_types = 7;
  filter_box_types = new kdu_uint32[max_filter_box_types];
  filter_box_types[0] = jp2_label_4cc;
  filter_box_types[1] = jp2_xml_4cc;
  filter_box_types[2] = jp2_iprights_4cc;
  filter_box_types[3] = jp2_number_list_4cc;
  filter_box_types[4] = jp2_roi_description_4cc;
  filter_box_types[5] = jp2_uuid_4cc;
  filter_box_types[6] = jp2_cross_reference_4cc;
  deleted_nodes = NULL;
  touched_head = touched_tail = NULL;
  have_link_nodes = false;
  simulate_write = false;
}

/*****************************************************************************/
/*                     jx_meta_manager::~jx_meta_manager                     */
/*****************************************************************************/

jx_meta_manager::~jx_meta_manager()
{
  tree->safe_delete();
  tree = NULL;
  for (int n=0; n < 32; n++)
    if (roi_scales[n] != NULL)
      delete roi_scales[n];
  if (filter_box_types != NULL)
    delete[] filter_box_types;
  jx_metanode *node;
  while ((node=deleted_nodes) != NULL)
    {
      deleted_nodes = node->next_sibling;
      delete node;
    }
}

/*****************************************************************************/
/*                     jx_meta_manager::test_box_filter                      */
/*****************************************************************************/

bool
  jx_meta_manager::test_box_filter(kdu_uint32 type)
{
  if (type == jp2_association_4cc)
    return true;
  if (num_filter_box_types == 0)
    return true; // Accept everything
  for (int n=0; n < num_filter_box_types; n++)
    if (type == filter_box_types[n])
      return true;
  return false;
}

/*****************************************************************************/
/*                           jx_meta_manager::link                           */
/*****************************************************************************/

void
  jx_meta_manager::link(jx_metanode *node)
{
  if (node->sequence_index == 0)
    {
      if (next_sequence_index == 0)
        next_sequence_index++; // Avoid assigning 0
      node->sequence_index = (this->next_sequence_index++);
    }

  last_added_node = node;
  if (node->rep_id == JX_NUMLIST_NODE)
    { // This is a number list node
      numlist_nodes.link(node);
    }
  else if (node->rep_id == JX_ROI_NODE)
    { // This is an ROI node
      kdu_dims region = node->regions->bounding_region.region;
      int max_dim = (region.size.x>region.size.y)?region.size.x:region.size.y;
      int scale_idx = 0;
      while ((max_dim > (JX_FINEST_SCALE_THRESHOLD<<scale_idx)) &&
             (scale_idx < 31))
        scale_idx++;
      jx_roigroup *roig = roi_scales[scale_idx];
      if (roig == NULL)
        roig = roi_scales[scale_idx] = new jx_roigroup(this,scale_idx,0);
      while ((region.pos.x >= roig->group_dims.size.x) ||
             (region.pos.y >= roig->group_dims.size.y))
        { // Need to create a new, larger root, with all existing groups
          // as subordinates
          jx_roigroup *tmp = new jx_roigroup(this,scale_idx,roig->level+1);
          tmp->sub_groups[0] = roig;
          roig = roig->parent = tmp;
          roi_scales[scale_idx] = roig;
        }
      jx_metagroup *mgrp = NULL;
      while (mgrp == NULL)
        {
          int p_x = region.pos.x - roig->group_dims.pos.x;
          int p_y = region.pos.y - roig->group_dims.pos.y;
          assert((p_x >= 0) && (p_y >= 0));
          p_x = p_x / roig->elt_size.x;
          p_y = p_y / roig->elt_size.y;
          assert((p_x < JX_ROIGROUP_SIZE) && (p_y < JX_ROIGROUP_SIZE));
          if (roig->level > 0)
            {
              jx_roigroup **sub = roig->sub_groups+p_y*JX_ROIGROUP_SIZE+p_x;
              if (*sub == NULL)
                {
                  *sub = new jx_roigroup(this,scale_idx,roig->level-1);
                  (*sub)->parent = roig;
                  (*sub)->group_dims.pos.x =
                    roig->group_dims.pos.x + p_x*roig->elt_size.x;
                  (*sub)->group_dims.pos.y =
                    roig->group_dims.pos.y + p_y*roig->elt_size.y;
                }
              roig = *sub;
            }
          else
            mgrp = roig->metagroups + p_y*JX_ROIGROUP_SIZE + p_x;
        }
      mgrp->roigroup = roig;
      mgrp->link(node);
    }
  else if (node->box_type != jp2_free_4cc)
    { // For all other node types, check if they are descended from numlist/ROI
      jx_metanode *test;
      for (test=node->parent; test != NULL; test=test->parent)
        if ((test->rep_id == JX_NUMLIST_NODE) ||
            (test->rep_id == JX_ROI_NODE))
          return; // Don't enter it onto any metagroup list
      
      // If we get here, we have an unassociated node
      unassociated_nodes.link(node);
    }
}

/*****************************************************************************/
/*                      jx_meta_manager::write_metadata                      */
/*****************************************************************************/

jp2_output_box *
  jx_meta_manager::write_metadata(int *i_param, void **addr_param)
{
  int next_scale_idx=0;
  jx_roigroup *roig=NULL;
  bool started_numlists=false;
  bool started_unassociated=false;
  kdu_coords roi_loc; // Location within current roigroup, if any
  int roi_offset=0; // Offset within current roigroup to `roi_loc'
  if (active_metagroup != NULL)
    {
      roig = active_metagroup->roigroup;
      if (roig == NULL)
        {
          next_scale_idx = 32; // We have already written all ROI metadata
          if (active_metagroup == &numlist_nodes)
            started_numlists = true;
          else if (active_metagroup == &unassociated_nodes)
            started_numlists = started_unassociated = true;
          else
            assert(0);
        }
      else
        {
          roi_offset = (int)(active_metagroup - roig->metagroups);
          assert((roi_offset >= 0) &&
                 (roi_offset < (JX_ROIGROUP_SIZE*JX_ROIGROUP_SIZE)));
          roi_loc.y = roi_offset / JX_ROIGROUP_SIZE;
          roi_loc.x = roi_offset - roi_loc.y*JX_ROIGROUP_SIZE;
        }
    }

  jp2_output_box sub, *interrupt=NULL;
  do {
      while (active_metagroup == NULL)
        { // Hunt for the next active metagroup
          assert(!active_out);
          if (roig == NULL)
            {
              if (next_scale_idx < 32)
                {
                  if ((roig = roi_scales[next_scale_idx++]) == NULL)
                    continue; // Try again with next scale, if there is one
                  roi_loc = kdu_coords(0,0);
                }
              else if (!started_numlists)
                {
                  started_numlists = true;
                  active_metagroup = &numlist_nodes;
                  break; // Found the metagroup we want
                }
              else if (!started_unassociated)
                {
                  started_unassociated = true;
                  active_metagroup = &unassociated_nodes;
                  active_metagroup->mark_for_writing();
                    // All other metagroups will be marked when we detect an
                    // `active_out' box which is not open, but the unassociated
                    // boxes are not written within any `active_out' superbox.
                  break; // Found the metagroup we want
                }
              else
                return NULL; // All done!!
            }
          else if (roi_loc.x < (JX_ROIGROUP_SIZE-1))
            { // Advance horizontally within current roigroup at current level
              roi_loc.x++;
            }
          else if (roi_loc.y < (JX_ROIGROUP_SIZE-1))
            { // Advance vertically within current roigroup at current level
              roi_loc.y++; roi_loc.x = 0;
            }
          else
            { // Close current group and go up to the parent
              roig->active_out.close();
              jx_roigroup *parent = roig->parent;
              if (parent != NULL)
                {
                  roi_loc = roig->group_dims.pos - parent->group_dims.pos;
                  roi_loc.x = roi_loc.x / parent->elt_size.x;
                  roi_loc.y = roi_loc.y / parent->elt_size.y;
                  assert((roi_loc.x >= 0) && (roi_loc.x < JX_ROIGROUP_SIZE) &&
                         (roi_loc.y >= 0) && (roi_loc.y < JX_ROIGROUP_SIZE));
                }
              roig = parent;
              continue; // Still need to advance within the parent group
            }

          // If we get here, we must be in an roigroup at some level
          assert(roig != NULL);
          roi_offset = roi_loc.x + roi_loc.y*JX_ROIGROUP_SIZE;
          while (roig->level > 0)
            { // Descend to the leaves
              if (roig->sub_groups[roi_offset] == NULL)
                break; // Need to go to next group at same/higher level
              roig = roig->sub_groups[roi_offset];
              roi_loc = kdu_coords(0,0);
              roi_offset = 0;
            }
          if (roig->level > 0)
            continue; // Advance to next roi/meta group at same/higher level
          active_metagroup = roig->metagroups + roi_offset;
        }

      if ((!active_out) && !started_unassociated)
        { // Writing for the active metagroup has not yet commenced
          if (!active_metagroup->mark_for_writing())
            { // Nothing to write; don't start an output context
              active_metagroup = NULL;
              continue;
            }
          char explanation[80]; // To go in the free box.
          explanation[0] = '\0';
          if (roig != NULL)
            { // Open boxes, as required to form the scale-space hierarchy
              bool all_open = roig->active_out.exists();
              while (!all_open)
                {
                  all_open = true;
                  jx_roigroup *pscan=roig;
                  while ((pscan->parent != NULL) &&
                         !pscan->parent->active_out)
                    { pscan = pscan->parent; all_open = false; }
                  if (pscan->parent != NULL)
                    {
                      pscan->active_out.open(&pscan->parent->active_out,
                                             jp2_association_4cc);
                      sprintf(explanation,
                              "ROI group: x0=%d; y0=%d; w=%d; h=%d",
                              pscan->group_dims.pos.x,
                              pscan->group_dims.pos.y,
                              pscan->group_dims.size.x,
                              pscan->group_dims.size.y);
                    }
                  else
                    {
                      target->open_top_box(&pscan->active_out,
                                           jp2_association_4cc,
                                           simulate_write);
                      sprintf(explanation,"ROI scale %d",next_scale_idx-1);
                    }
                  pscan->active_out.write_header_last();
                       // So that box locations can be computed directly
                  sub.open(&pscan->active_out,jp2_free_4cc);
                  sub.write((kdu_byte *) explanation,(int)strlen(explanation));
                  sub.close();
                }
              active_metagroup_out_pos = target->get_file_pos();
              active_out.open(&roig->active_out,jp2_association_4cc);
              sprintf(explanation,"ROI group: x0=%d; y0=%d; w=%d; h=%d",
                      roig->group_dims.pos.x+roig->elt_size.x*roi_loc.x,
                      roig->group_dims.pos.y+roig->elt_size.y*roi_loc.y,
                      roig->elt_size.x,roig->elt_size.y);
            }
          else
            {
              active_metagroup_out_pos =
                target->open_top_box(&active_out,jp2_association_4cc,
                                     simulate_write);
              strcpy(explanation,"Metadata associated with image entities");
            }
          sub.open(&active_out,jp2_free_4cc);
          sub.write((kdu_byte *) explanation,(int)strlen(explanation));
          active_metagroup_out_pos += sub.get_box_length();
          sub.close();
        }

      if (active_out.exists())
        interrupt =
          tree->write(&active_out,target,active_metagroup,i_param,addr_param);
      else
        { // Write unassociated nodes at the top level
          assert(active_metagroup == &unassociated_nodes);
          interrupt =
            tree->write(NULL,target,active_metagroup,i_param,addr_param);
        }
      if (interrupt == NULL)
        {
          if (active_out.exists() && simulate_write)
            {
              active_metagroup_out_pos += active_out.get_header_length();
              tree->discover_output_box_locations(active_metagroup_out_pos,
                                                  active_metagroup);
            }
          active_out.close();
          active_metagroup = NULL; // Finished with this group; move on to next
        }
    } while (interrupt == NULL);

  return interrupt;
}


/* ========================================================================= */
/*                            jpx_meta_manager                               */
/* ========================================================================= */

/*****************************************************************************/
/*                      jpx_meta_manager::access_root                        */
/*****************************************************************************/

jpx_metanode
  jpx_meta_manager::access_root()
{
  jpx_metanode result;
  if (state != NULL)
    result = jpx_metanode(state->tree);
  return result;
}

/*****************************************************************************/
/*                      jpx_meta_manager::locate_node                        */
/*****************************************************************************/

jpx_metanode
  jpx_meta_manager::locate_node(kdu_long file_pos)
{
  jx_metaloc *metaloc = NULL;
  if (state != NULL)
    {
      metaloc = state->metaloc_manager.get_locator(file_pos,false);
      if ((metaloc != NULL) && (metaloc->target != NULL) &&
          (metaloc->target->rep_id == JX_CROSSREF_NODE) &&
          (metaloc->target->crossref != NULL) &&
          (metaloc->target->crossref->metaloc == metaloc))
        metaloc = NULL; // Locator holding information for an unresolved link
    }
  return jpx_metanode((metaloc == NULL)?NULL:(metaloc->target));
}

/*****************************************************************************/
/*                   jpx_meta_manager::get_touched_nodes                     */
/*****************************************************************************/

jpx_metanode
  jpx_meta_manager::get_touched_nodes()
{
  jx_metanode *node=NULL;
  if ((state != NULL) && ((node=state->touched_head) != NULL))
    {
      assert(node->prev_touched == NULL);
      if ((state->touched_head = node->next_touched) == NULL)
        {
          assert(node == state->touched_tail);
          state->touched_tail = NULL;
        }
      else
        state->touched_head->prev_touched = NULL;
      node->next_touched = NULL;
    }
  return jpx_metanode(node);
}

/*****************************************************************************/
/*                  jpx_meta_manager::peek_touched_nodes                     */
/*****************************************************************************/

jpx_metanode
  jpx_meta_manager::peek_touched_nodes(kdu_uint32 box_type,
                                       jpx_metanode last_peeked)
{
  jx_metanode *node=NULL;
  if (state != NULL)
    {
      node = last_peeked.state;
      if (node == NULL)
        node = state->touched_head;
      else if ((node->prev_touched == NULL) && (node != state->touched_head))
        node = NULL;
      else
        node = node->next_touched;
      for (; node != NULL; node=node->next_touched)
        if ((box_type == 0) || (node->box_type == box_type))
          break;
    }
  return jpx_metanode(node);
}

/*****************************************************************************/
/*                         jpx_meta_manager::copy                            */
/*****************************************************************************/

void
  jpx_meta_manager::copy(jpx_meta_manager src)
{
  assert((state != NULL) && (src.state != NULL));
  int dummy_count;
  src.access_root().count_descendants(dummy_count); // Finish top level parsing
  jpx_metanode tree = access_root();
  jx_metanode *scan;
  for (scan=src.state->tree->head; scan != NULL; scan=scan->next_sibling)
    tree.add_copy(jpx_metanode(scan),true);
}

/*****************************************************************************/
/*                  jpx_meta_manager::reset_copy_locators                    */
/*****************************************************************************/

void
  jpx_meta_manager::reset_copy_locators(jpx_metanode src, bool recursive,
                                        bool fixup_unresolved_links)
{
  if (src.state->manager != state)
    fixup_unresolved_links = false;
  if (src.state->linked_from != NULL)
    { // Source object has links so we may have a copy locator for it
      jx_metaloc *metaloc =
        state->metaloc_manager.get_copy_locator(src.state,false);
      if (metaloc != NULL)
        {
          jx_crossref *scan, *next;
          if ((metaloc->target != NULL) &&
              (metaloc->target->rep_id == JX_CROSSREF_NODE) &&
              (metaloc->target->crossref->metaloc == metaloc))
            for (scan=metaloc->target->crossref; scan != NULL; scan=next)
              { // These cross-references all failed to resolve during a
                // previous copy.
                next = scan->next_link;
                scan->next_link = NULL;
                scan->metaloc = NULL;
                if (fixup_unresolved_links)
                  {
                    scan->link = src.state;
                    if (src.state->linked_from == NULL)
                      src.state->linked_from = scan;
                    else
                      scan->append_to_list(src.state->linked_from);                      
                  }
              }
          metaloc->target = NULL;
        }
    }
  
  if (recursive)
    {
      jx_metanode *scan;
      for (scan=src.state->head; scan != NULL; scan=scan->next_sibling)
        reset_copy_locators(jpx_metanode(scan),true,fixup_unresolved_links);
    }
}

/*****************************************************************************/
/*                    jpx_meta_manager::set_box_filter                       */
/*****************************************************************************/

void
  jpx_meta_manager::set_box_filter(int num_types, kdu_uint32 *types)
{
  assert((state != NULL) && (state->filter_box_types != NULL));
  if (num_types > state->max_filter_box_types)
    {
      int new_max_types = state->max_filter_box_types + num_types;
      kdu_uint32 *new_types = new kdu_uint32[new_max_types];
      delete[] state->filter_box_types;
      state->filter_box_types = new_types;
      state->max_filter_box_types = new_max_types;
      for (int n=0; n < num_types; n++)
        state->filter_box_types[n] = types[n];
      state->num_filter_box_types = num_types;
    }
}

/*****************************************************************************/
/*                     jpx_meta_manager::load_matches                        */
/*****************************************************************************/

bool
  jpx_meta_manager::load_matches(int num_codestreams,
                                 const int codestream_indices[],
                                 int num_compositing_layers,
                                 const int layer_indices[])
{
  jx_metanode *last_one = state->last_added_node;
  // Start by reading as many top-level boxes as possible
  while (state->source->parse_next_top_level_box());

  // Now finish parsing all relevant codestream and layer headers
  if (num_codestreams < 0)
    {
      num_codestreams = state->source->get_num_codestreams();
      codestream_indices = NULL;
    }
  if (num_compositing_layers < 0)
    {
      num_compositing_layers = state->source->get_num_compositing_layers();
      layer_indices = NULL;
    }
  int n;
  for (n=0; n < num_codestreams; n++)
    {
      int idx = (codestream_indices==NULL)?n:(codestream_indices[n]);
      jx_codestream_source *cs =
        state->source->get_codestream(idx);
      if (cs != NULL)
        cs->finish(false);
    }
  for (n=0; n < num_compositing_layers; n++)
    {
      int idx = (layer_indices==NULL)?n:(layer_indices[n]);
      jx_layer_source *ls =
        state->source->get_compositing_layer(idx);
      if (ls != NULL)
        ls->finish();
    }

  // Finally, recursively scan through the metadata, finishing as many
  // nodes as possible.
  state->tree->load_recursive();

  return (last_one != state->last_added_node);
}

/*****************************************************************************/
/*                   jpx_meta_manager::enumerate_matches                     */
/*****************************************************************************/

jpx_metanode
  jpx_meta_manager::enumerate_matches(jpx_metanode last_node,
                                      int codestream_idx,
                                      int compositing_layer_idx,
                                      bool applies_to_rendered_result,
                                      kdu_dims region, int min_size,
                                      bool exclude_region_numlists,
                                      bool ignore_missing_numlist_info)
{
  if (state == NULL)
    return jpx_metanode(NULL);

  jx_metanode *node = last_node.state;
  if (!region.is_empty())
    { // Search in the ROI hierarchy
      int next_scale_idx = 0;
      jx_metagroup *mgrp=NULL;
      jx_roigroup *roig=NULL;
      kdu_coords loc; // Location within current ROI group object
      int offset=0; // loc.x + loc.y*JX_ROIGROUP_SIZE

      // min & lim give the range of ROI start coords which could be compatible
      kdu_coords lim = region.pos + region.size;
      kdu_coords min;
      if (node != NULL)
        { // Find out where we currently are and configure `min'
          mgrp = node->metagroup;
          roig = mgrp->roigroup;
          next_scale_idx = roig->scale_idx+1;
          offset = (int)(mgrp - roig->metagroups);
          assert((offset>=0) && (offset<(JX_ROIGROUP_SIZE*JX_ROIGROUP_SIZE)));
          loc.y = offset / JX_ROIGROUP_SIZE;
          loc.x = offset - loc.y*JX_ROIGROUP_SIZE;
          min = region.pos;
          min.x -= (JX_FINEST_SCALE_THRESHOLD<<roig->scale_idx)-1;
          min.y -= (JX_FINEST_SCALE_THRESHOLD<<roig->scale_idx)-1;
          node = node->next_link;
        }
      while ((roig != NULL) || (next_scale_idx < 32))
        {
          if (node == NULL)
            { // Need to advance to next list, group or scale
              if (roig == NULL)
                { // Advance to next scale
                  roig = state->roi_scales[next_scale_idx++];
                  if (roig == NULL)
                    continue;
                  min = region.pos;
                  min.x -= (JX_FINEST_SCALE_THRESHOLD<<roig->scale_idx)-1;
                  min.y -= (JX_FINEST_SCALE_THRESHOLD<<roig->scale_idx)-1;
                  loc = min - roig->group_dims.pos;
                  loc.x = (loc.x<0)?0:(loc.x/roig->elt_size.x);
                  loc.y = (loc.y<0)?0:(loc.y/roig->elt_size.y);
                  if ((loc.x>=JX_ROIGROUP_SIZE) || (loc.y>=JX_ROIGROUP_SIZE))
                    { // No intersection with this group
                      roig = NULL;
                      continue;
                    }
                }
              else if (((loc.x+1) < JX_ROIGROUP_SIZE) &&
                       (lim.x > (roig->group_dims.pos.x +
                                 (loc.x+1)*roig->elt_size.x)))
                loc.x++;
              else if (((loc.y+1) < JX_ROIGROUP_SIZE) &&
                       (lim.y > (roig->group_dims.pos.y +
                                 (loc.y+1)*roig->elt_size.y)))
                { 
                  loc.y++;
                  loc.x = min.x - roig->group_dims.pos.x;
                  loc.x = (loc.x < 0)?0:(loc.x / roig->elt_size.x);
                }
              else
                { // Go to group's parent
                  jx_roigroup *parent = roig->parent;
                  if (parent != NULL)
                    {
                      min = region.pos;
                      min.x-=(JX_FINEST_SCALE_THRESHOLD<<parent->scale_idx)-1;
                      min.y-=(JX_FINEST_SCALE_THRESHOLD<<parent->scale_idx)-1;
                      loc = roig->group_dims.pos - parent->group_dims.pos;
                      loc.x = loc.x / parent->elt_size.x;
                      loc.y = loc.y / parent->elt_size.y;
                      assert((loc.x >= 0) && (loc.x < JX_ROIGROUP_SIZE) &&
                             (loc.y >= 0) && (loc.y < JX_ROIGROUP_SIZE));
                    }
                  roig = parent;
                  continue; // Still need to advance within the parent group
                }

              // If we get here, we are in the next compatible metagroup or
              // roigroup at some level.  Descend back down to the metagroup
              // level, if possible.
              offset = loc.x + loc.y*JX_ROIGROUP_SIZE;
              while (roig->level > 0)
                {
                  if (roig->sub_groups[offset] == NULL)
                    break; // Need to go to next group at same/higher level
                  roig = roig->sub_groups[offset];
                  min = region.pos;
                  min.x -= (JX_FINEST_SCALE_THRESHOLD<<roig->scale_idx)-1;
                  min.y -= (JX_FINEST_SCALE_THRESHOLD<<roig->scale_idx)-1;
                  loc = min - roig->group_dims.pos;
                  loc.x = (loc.x<0)?0:(loc.x/roig->elt_size.x);
                  loc.y = (loc.y<0)?0:(loc.y/roig->elt_size.y);
                  assert((loc.x<JX_ROIGROUP_SIZE) && (loc.y<JX_ROIGROUP_SIZE));
                  offset = loc.x + loc.y*JX_ROIGROUP_SIZE;
                }
              if (roig->level > 0)
                continue; // Advance to next group/list at same/higher level
              mgrp = roig->metagroups + offset;
              node = mgrp->head;
              if (node == NULL)
                continue;
            }

          // If we get here, `node' is non-NULL.  See if it conforms to the
          // constraints or not.
          assert((node->rep_id == JX_ROI_NODE) && (node->regions != NULL));
          bool found_match = false;
          if ((node->regions->max_width >= min_size) &&
              node->regions->bounding_region.region.intersects(region))
            { // Found a spatial match; look for numlist match also
              if ((codestream_idx < 0) && (compositing_layer_idx < 0) &&
                  !applies_to_rendered_result)
                found_match = true;
              else
                {
                  jx_metanode *scan=node->parent;
                  while ((scan != NULL) &&
                         !((scan->rep_id == JX_NUMLIST_NODE) &&
                           (scan->numlist != NULL)))
                    scan = scan->parent;
                  bool outcome_known = false;
                  if (scan != NULL)
                    {
                      int n;
                      if (applies_to_rendered_result)
                        {
                          if (scan->numlist->rendered_result)
                            found_match = true;
                          else
                            outcome_known = true;
                        }
                      if ((codestream_idx >= 0) && (!outcome_known))
                        {
                          for (n=0; n < scan->numlist->num_codestreams; n++)
                            if (scan->numlist->codestream_indices[n] ==
                                codestream_idx)
                              break;
                          if (n < scan->numlist->num_codestreams)
                            found_match = true;
                          else if ((n > 0) || !ignore_missing_numlist_info)
                            { found_match = false; outcome_known = true; }
                        }
                      if ((compositing_layer_idx >= 0) && (!outcome_known))
                        {
                          for (n=0;
                               n < scan->numlist->num_compositing_layers; n++)
                            if (scan->numlist->layer_indices[n] ==
                                compositing_layer_idx)
                              break;
                          if (n < scan->numlist->num_compositing_layers)
                            found_match = true;
                          else if ((n > 0) || !ignore_missing_numlist_info)
                            { found_match = false; outcome_known = true; }
                        }
                    }
                }
            }
          if (found_match)
            return jpx_metanode(node);
          node = node->next_link;
        }
    }
  else if ((codestream_idx >= 0) || (compositing_layer_idx >= 0) ||
           applies_to_rendered_result)
    { // Search in numlist group
      while (1)
        {
          if (node == NULL)
            node = state->numlist_nodes.head;
          else
            {
              assert(node->metagroup == &state->numlist_nodes);
              node = node->next_link;
            }
          if (node == NULL)
            break;
          if ((node->rep_id != JX_NUMLIST_NODE) || (node->numlist == NULL))
            continue;
          if (exclude_region_numlists)
            {
              jx_metanode *dp;
              for (dp=node->head; dp != NULL; dp=dp->next_link)
                if (dp->rep_id != JX_ROI_NODE)
                  break;
              if (dp == NULL)
                continue; // Node contains no non-ROI descendants
            }
          if (applies_to_rendered_result && !node->numlist->rendered_result)
            continue; // Failed to match numlist
          int n;
          if (codestream_idx >= 0)
            {
              for (n=0; n < node->numlist->num_codestreams; n++)
                if (node->numlist->codestream_indices[n]==codestream_idx)
                  break;
              if (n == node->numlist->num_codestreams)
                continue; // No match found
            }
          if (compositing_layer_idx >= 0)
            {
              for (n=0; n < node->numlist->num_compositing_layers; n++)
                if (node->numlist->layer_indices[n]==compositing_layer_idx)
                  break;
              if (n == node->numlist->num_compositing_layers)
                continue; // No match found
            }
          return jpx_metanode(node); // Found a complete match
        }
    }
  else
    { // Search in unassociated group
      if (node == NULL)
        node = state->unassociated_nodes.head;
      else
        {
          assert(node->metagroup == &state->unassociated_nodes);
          node = node->next_link;
        }
    }

  return jpx_metanode(node);
}

/*****************************************************************************/
/*                      jpx_meta_manager::insert_node                        */
/*****************************************************************************/

jpx_metanode
  jpx_meta_manager::insert_node(int num_codestreams,
                                const int codestream_indices[],
                                int num_compositing_layers,
                                const int layer_indices[],
                                bool applies_to_rendered_result,
                                int num_regions, const jpx_roi *regions)
{
  assert(state != NULL);
  assert((num_codestreams >= 0) && (num_compositing_layers >= 0));
  if ((num_regions > 0) && (num_codestreams == 0))
    { KDU_ERROR_DEV(e,30); e <<
        KDU_TXT("You may not use `jpx_meta_manager::insert_node' to "
        "insert a region-specific metadata node which is not also associated "
        "with at least one codestream.  The reason is that ROI description "
        "boxes have meaning only when associated with codestreams, as "
        "described in the JPX standard.");
    }
  if ((num_codestreams == 0) && (num_compositing_layers == 0) &&
      !applies_to_rendered_result)
    return jpx_metanode(state->tree);

  int n, k, idx;
  jx_metanode *scan;
  for (scan=state->tree->head; scan != NULL; scan=scan->next_sibling)
    {
      if (!((scan->flags & JX_METANODE_BOX_COMPLETE) &&
            (scan->rep_id == JX_NUMLIST_NODE)))
        continue;
      if (applies_to_rendered_result != scan->numlist->rendered_result)
        continue;

      // Check that `codestream_indices' are all in the numlist node
      for (n=0; n < num_codestreams; n++)
        {
          idx = codestream_indices[n];
          for (k=0; k < scan->numlist->num_codestreams; k++)
            if (scan->numlist->codestream_indices[k] == idx)
              break;
          if (k == scan->numlist->num_codestreams)
            break; // No match
        }
      if (n < num_codestreams)
        continue;

      // Check that numlist node's codestreams are all in `codestrem_indices'
      for (n=0; n < scan->numlist->num_codestreams; n++)
        {
          idx = scan->numlist->codestream_indices[n];
          for (k=0; k < num_codestreams; k++)
            if (codestream_indices[k] == idx)
              break;
          if (k == num_codestreams)
            break; // No match
        }
      if (n < scan->numlist->num_codestreams)
        continue;

      // Check that `layer_indices' are all in the numlist node
      for (n=0; n < num_compositing_layers; n++)
        {
          idx = layer_indices[n];
          for (k=0; k < scan->numlist->num_compositing_layers; k++)
            if (scan->numlist->layer_indices[k] == idx)
              break;
          if (k == scan->numlist->num_compositing_layers)
            break; // No match
        }
      if (n < num_compositing_layers)
        continue;

      // Check that the numlist node's layers are all in `layer_indices'
      for (n=0; n < scan->numlist->num_compositing_layers; n++)
        {
          idx = scan->numlist->layer_indices[n];
          for (k=0; k < num_compositing_layers; k++)
            if (layer_indices[k] == idx)
              break;
          if (k == num_compositing_layers)
            break; // No match
        }
      if (n < scan->numlist->num_compositing_layers)
        continue;

      break; // We have found a compatible number list
    }

  jpx_metanode result(scan);
  if (scan == NULL)
    result = access_root().add_numlist(num_codestreams,codestream_indices,
                                       num_compositing_layers,layer_indices,
                                       applies_to_rendered_result);
  if (num_regions == 0)
    return result;

  return result.add_regions(num_regions,regions);
}


/* ========================================================================= */
/*                              jx_registration                              */
/* ========================================================================= */

/*****************************************************************************/
/*                           jx_registration::init                           */
/*****************************************************************************/

void
  jx_registration::init(jp2_input_box *creg)
{
  if (codestreams != NULL)
    { KDU_ERROR(e,31); e <<
        KDU_TXT("JPX data source appears to contain multiple "
        "JPX Codestream Registration (creg) boxes within the same "
        "compositing layer header (jplh) box.");
    }
  final_layer_size.x = final_layer_size.y = 0; // To be calculated later
  kdu_uint16 xs, ys;
  if (!(creg->read(xs) && creg->read(ys) && (xs > 0) && (ys > 0)))
    { KDU_ERROR(e,32); e <<
        KDU_TXT("Malformed Codestream Registration (creg) box found "
        "in JPX data source.  Insufficient or illegal fields encountered.");
    }
  denominator.x = (int) xs;
  denominator.y = (int) ys;
  int box_bytes = (int) creg->get_remaining_bytes();
  if ((box_bytes % 6) != 0)
    { KDU_ERROR(e,33); e <<
        KDU_TXT("Malformed Codestream Registration (creg) box found "
        "in JPX data source.  Box size does not seem to be equal to 4+6k "
        "where k must be the number of referenced codestreams.");
    }
  num_codestreams = max_codestreams = (box_bytes / 6);
  codestreams = new jx_layer_stream[max_codestreams];
  for (int c=0; c < num_codestreams; c++)
    {
      jx_layer_stream *str = codestreams + c;
      kdu_uint16 cdn;
      kdu_byte xr, yr, xo, yo;
      if (!(creg->read(cdn) && creg->read(xr) && creg->read(yr) &&
            creg->read(xo) && creg->read(yo)))
        assert(0); // Should not be possible
      if ((xr == 0) || (yr == 0))
        { KDU_ERROR(e,34); e <<
            KDU_TXT("Malformed Codestream Registration (creg) box "
            "found in JPX data source.  Illegal (zero-valued) resolution "
            "parameters found for codestream " << cdn << ".");
        }
      if ((denominator.x <= (int) xo) || (denominator.y <= (int) yo))
        { KDU_ERROR(e,35); e <<
            KDU_TXT("Malformed Codestream Registration (creg) box "
            "found in JPX data source.  Alignment offsets must be strictly "
            "less than the denominator (point density) parameters, as "
            "explained in the JPX standard (accounting for corrigenda).");
        }
      str->codestream_id = (int) cdn;
      str->sampling.x = xr;
      str->sampling.y = yr;
      str->alignment.x = xo;
      str->alignment.y = yo;
    }
  creg->close();
}

/*****************************************************************************/
/*                     jx_registration::finalize (reading)                   */
/*****************************************************************************/

void
  jx_registration::finalize(int channel_id)
{
  if (codestreams == NULL)
    { // Create default registration info
      num_codestreams = max_codestreams = 1;
      codestreams = new jx_layer_stream[1];
      codestreams->codestream_id = channel_id;
      codestreams->sampling = kdu_coords(1,1);
      codestreams->alignment = kdu_coords(0,0);
      denominator = kdu_coords(1,1);
    }
}

/*****************************************************************************/
/*                     jx_registration::finalize (writing)                   */
/*****************************************************************************/

void
  jx_registration::finalize(j2_channels *channels, int layer_idx)
{
  if ((denominator.x == 0) || (denominator.y == 0))
    denominator = kdu_coords(1,1);
  int n, k, c, cid;
  for (k=0; k < channels->num_colours; k++)
    {
      j2_channels::j2_channel *cp = channels->channels + k;
      for (c=0; c < 3; c++)
        {
          cid = cp->codestream_idx[c];
          if (cid < 0)
            continue;
          for (n=0; n < num_codestreams; n++)
            if (codestreams[n].codestream_id == cid)
              break;
          if (n == num_codestreams)
            { // Add a new one
              if (n >= max_codestreams)
                { // Allocate new array
                  int new_max_cs = max_codestreams*2 + 2;
                  jx_layer_stream *buf = new jx_layer_stream[new_max_cs];
                  for (n=0; n < num_codestreams; n++)
                    buf[n] = codestreams[n];
                  if (codestreams != NULL)
                    delete[] codestreams;
                  codestreams = buf;
                  max_codestreams = new_max_cs;
                }
              num_codestreams++;
              codestreams[n].codestream_id = cid;
              codestreams[n].sampling = denominator;
              codestreams[n].alignment = kdu_coords(0,0);
            }
        }
    }
  
  // Before finishing up, we should check to see whether codestream
  // registration has been specified for any unused codestreams.
  for (n=0; n < num_codestreams; n++)
    {
      int cid = codestreams[n].codestream_id;
      bool found = false;
      for (k=0; (k < channels->num_colours) && !found; k++)
        {
          j2_channels::j2_channel *cp = channels->channels + k;
          for (c=0; c < 3; c++)
            if (cid == cp->codestream_idx[c])
              found = true;
        }
      if (!found)
        { KDU_ERROR_DEV(e,0x05011001); e <<
          KDU_TXT("Registration information has been supplied via the "
                  "`jpx_layer_target::set_codestream_registration' for a "
                  "codestream which is not used by any channel defined for "
                  "the compositing layer in question!  The codestream "
                  "in question has (zero-based) index ") << cid <<
          KDU_TXT(" and the compositing layer has (zero-based) index ") <<
          layer_idx << ".";
        }
    }
}

/*****************************************************************************/
/*                        jx_registration::save_boxes                        */
/*****************************************************************************/

void
  jx_registration::save_box(jp2_output_box *super_box)
{
  assert(num_codestreams > 0);
  int n;
  jx_layer_stream *str;

  jp2_output_box creg;
  creg.open(super_box,jp2_registration_4cc);
  creg.write((kdu_uint16) denominator.x);
  creg.write((kdu_uint16) denominator.y);
  for (n=0; n < num_codestreams; n++)
    {
      str = codestreams + n;
      int cdn = str->codestream_id;
      int xr = str->sampling.x;
      int yr = str->sampling.y;
      int xo = str->alignment.x;
      int yo = str->alignment.y;
      if ((cdn < 0) || (cdn >= (1<<16)) || (xr <= 0) || (yr <= 0) ||
          (xr > 255) || (yr > 255) || (xo < 0) || (yo < 0) ||
          (xo >= denominator.x) || (yo >= denominator.y))
        { KDU_ERROR_DEV(e,36); e <<
            KDU_TXT("One or more codestreams registration parameters "
            "supplied using `jpx_layer_target::set_codestream_registration' "
            "cannot be recorded in a legal JPX Codestream Registration (creg) "
            "box.");
        }
      if (xo > 255)
        xo = 255;
      if (yo > 255)
        yo = 255;
      creg.write((kdu_uint16) cdn);
      creg.write((kdu_byte) xr);
      creg.write((kdu_byte) yr);
      creg.write((kdu_byte) xo);
      creg.write((kdu_byte) yo);
    }
  creg.close();
}


/* ========================================================================= */
/*                            jx_codestream_source                           */
/* ========================================================================= */

/*****************************************************************************/
/*                    jx_codestream_source::donate_chdr_box                  */
/*****************************************************************************/

void
  jx_codestream_source::donate_chdr_box(jp2_input_box &src)
{
  if (restrict_to_jp2)
    { src.close(); return; }
  assert((!header_loc) && (!chdr) && (!sub_box));
  chdr.transplant(src);
  header_loc = chdr.get_locator();
  finish(false);
}

/*****************************************************************************/
/*                 jx_codestream_source::donate_codestream_box               */
/*****************************************************************************/

void
  jx_codestream_source::donate_codestream_box(jp2_input_box &src)
{
  assert(!stream_loc);
  stream_box.transplant(src);
  stream_loc = stream_box.get_locator();
  if ((stream_box.get_box_type() == jp2_fragment_table_4cc) &&
      !parse_fragment_list())
    return;
  stream_main_header_found =
    (!stream_box.has_caching_source()) || stream_box.set_codestream_scope(0);
}

/*****************************************************************************/
/*                  jx_codestream_source::parse_fragment_list                */
/*****************************************************************************/

bool
  jx_codestream_source::parse_fragment_list()
{
  if (fragment_list != NULL)
    return true;
  assert(stream_box.get_box_type() == jp2_fragment_table_4cc);
  if (chdr.exists())
    return false; // Can't re-deploy `sub_box' until codestream header parsed
  if (!stream_box.is_complete())
    return false;
  
  while ((sub_box.exists() || sub_box.open(&stream_box)))
    {
      if (sub_box.get_box_type() != jp2_fragment_list_4cc)
        { // Skip over irrelevant sub-box
          sub_box.close();
          continue;
        }
      if (!sub_box.is_complete())
        break;
      fragment_list = new jx_fragment_list;
      fragment_list->init(&sub_box);
      sub_box.close();
      stream_box.close();
      stream_box.open_as(jpx_fragment_list(fragment_list),
                         owner->get_data_references(),
                         ultimate_src,jp2_codestream_4cc);
      return true;
    }
  return false; 
}

/*****************************************************************************/
/*                        jx_codestream_source::finish                       */
/*****************************************************************************/

bool
  jx_codestream_source::finish(bool need_embedded_codestream)
{
  if (metadata_finished &&
      !(need_embedded_codestream && stream_loc.is_null()))
    return true;

  // Parse top level boxes as required
  while (((!header_loc) || (need_embedded_codestream && !stream_loc)) &&
         (!restrict_to_jp2) && !owner->is_top_level_complete())
    if (!owner->parse_next_top_level_box())
      break;

  if (chdr.exists())
    { // Parse as much as we can of the codestream header box
      bool chdr_complete = chdr.is_complete();
      while (sub_box.exists() || sub_box.open(&chdr))
        {
          bool sub_complete = sub_box.is_complete();
          if (sub_box.get_box_type() == jp2_image_header_4cc)
            {
              if (!sub_complete)
                break;
              dimensions.init(&sub_box);
            }
          else if (sub_box.get_box_type() == jp2_bits_per_component_4cc)
            {
              if (!sub_complete)
                break;
              dimensions.process_bpcc_box(&sub_box);
              have_bpcc = true;
            }
          else if (sub_box.get_box_type() == jp2_palette_4cc)
            {
              if (!sub_complete)
                break;
              palette.init(&sub_box);
            }
          else if (sub_box.get_box_type() == jp2_component_mapping_4cc)
            {
              if (!sub_complete)
                break;
              component_map.init(&sub_box);
            }
          else if (owner->test_box_filter(sub_box.get_box_type()))
            { // Put into the metadata management system
              if (metanode == NULL)
                {
                  jx_metanode *tree = owner->get_metatree();
                  metanode = tree->add_numlist(1,&id,0,NULL,false);
                  metanode->flags |= JX_METANODE_EXISTING;
                  metanode->read_state = new jx_metaread;
                  metanode->read_state->codestream_source = this;
                }
              int dbin_nest =
                (sub_box.get_locator().get_databin_id()==0)?1:0;
              metanode->add_descendant()->donate_input_box(sub_box,dbin_nest);
              assert(!sub_box.exists());
            }
          else
            sub_box.close(); // Skip it
        }
          
      if (sub_box.exists())
        return false;

      // See if we have enough of codestream header to finish parsing it
      if (chdr_complete)
        {
          chdr.close();
          if (metanode != NULL)
            {
              metanode->flags |= JX_METANODE_DESCENDANTS_KNOWN;
              metanode->update_completed_descendants();
              delete metanode->read_state;
              metanode->read_state = NULL;
            }
        }
      else if (dimensions.is_initialized() && have_bpcc &&
               palette.is_initialized() && component_map.is_initialized())
        { // Donate `chdr' to the metadata management machinery to extract
          // any remaining boxes if it can.
          if (metanode == NULL)
            {
              jx_metanode *tree = owner->get_metatree();
              metanode = tree->add_numlist(1,&id,0,NULL,false);
              metanode->flags |= JX_METANODE_EXISTING;
              metanode->read_state = new jx_metaread;
            }
          metanode->read_state->codestream_source = NULL;
          metanode->read_state->asoc.transplant(chdr);
          assert(!chdr);
        }
    }
  
  if (chdr.exists() ||
      ((!restrict_to_jp2) && (!header_loc) &&
       !owner->is_top_level_complete()))
    return false; // Still waiting for the codestream header box

  if (!metadata_finished)
    {
      if (!dimensions.is_initialized())
        { // Dimensions information must be in default header box
          j2_dimensions *dflt =
            owner->get_default_dimensions();
          if (dflt == NULL)
            return false;
          if (!dflt->is_initialized())
            { KDU_ERROR(e,37); e <<
                KDU_TXT("JPX source contains no image header box for "
                "a codestream.  The image header (ihdr) box cannot be found in "
                "a codestream header (chdr) box, and does not exist within a "
                "default JP2 header (jp2h) box.");
            }
          dimensions.copy(dflt);
        }

      if (!palette.is_initialized())
        { // Palette info might be in default header box
          j2_palette *dflt = owner->get_default_palette();
          if (dflt == NULL)
            return false;
          if (dflt->is_initialized())
            palette.copy(dflt);
        }

      if (palette.is_initialized() && !component_map.is_initialized())
        { // Component mapping info must be in default header box
          j2_component_map *dflt =
            owner->get_default_component_map();
          if (dflt == NULL)
            return false;
          if (!dflt->is_initialized())
            { KDU_ERROR(e,38); e <<
                KDU_TXT("JPX source contains a codestream with a palette "
                "(pclr) box, but no component mapping (cmap) box.  This "
                "illegal situation has been detected after examining both the "
                "codestream header (chdr) box, if any, for that codestream, "
                "and the default JP2 header (jp2h) box, if any.");
            }
          component_map.copy(dflt);
        }

      dimensions.finalize();
      palette.finalize();
      component_map.finalize(&dimensions,&palette);
      metadata_finished = true;
    }

  if (need_embedded_codestream && !stream_loc)
    return false;
  return true;
}


/* ========================================================================= */
/*                           jpx_codestream_source                           */
/* ========================================================================= */

/*****************************************************************************/
/*                  jpx_codestream_source::get_codestream_id                 */
/*****************************************************************************/

int
  jpx_codestream_source::get_codestream_id()
{
  assert((state != NULL) && state->metadata_finished);
  return state->id;
}

/*****************************************************************************/
/*                   jpx_codestream_source::get_header_loc                   */
/*****************************************************************************/

jp2_locator
  jpx_codestream_source::get_header_loc()
{
  assert((state != NULL) && state->metadata_finished);
  return state->header_loc;
}

/*****************************************************************************/
/*                jpx_codestream_source::access_fragment_list                */
/*****************************************************************************/

jpx_fragment_list
  jpx_codestream_source::access_fragment_list()
{
  assert(state != NULL);
  if (state->is_stream_ready() && (state->fragment_list != NULL))
    return jpx_fragment_list(state->fragment_list);
  return jpx_fragment_list(NULL);
}

/*****************************************************************************/
/*                 jpx_codestream_source::access_dimensions                  */
/*****************************************************************************/

jp2_dimensions
  jpx_codestream_source::access_dimensions(bool finalize_compatibility)
{
  assert((state != NULL) && state->metadata_finished);
  jp2_dimensions result(&state->dimensions);
  if (finalize_compatibility && (!state->compatibility_finalized))
    {
      jpx_input_box *src = open_stream();
      if (src != NULL)
        {
          kdu_codestream cs;
          try {
              cs.create(src);
              result.finalize_compatibility(cs.access_siz());
            }
          catch (kdu_exception)
            { }
          if (cs.exists())
            cs.destroy();
          src->close();
          state->compatibility_finalized = true;
        }
    }
  return result;
}

/*****************************************************************************/
/*                   jpx_codestream_source::access_palette                   */
/*****************************************************************************/

jp2_palette
  jpx_codestream_source::access_palette()
{
  assert((state != NULL) && state->metadata_finished);
  return jp2_palette(&state->palette);
}

/*****************************************************************************/
/*                   jpx_codestream_source::get_stream_loc                   */
/*****************************************************************************/

jp2_locator
  jpx_codestream_source::get_stream_loc()
{
  assert((state != NULL) && state->metadata_finished);
  return state->stream_loc;
}

/*****************************************************************************/
/*                    jpx_codestream_source::stream_ready                    */
/*****************************************************************************/

bool
  jpx_codestream_source::stream_ready()
{
  assert(state != NULL);
  return state->is_stream_ready();
}

/*****************************************************************************/
/*                    jpx_codestream_source::open_stream                     */
/*****************************************************************************/

jpx_input_box *
  jpx_codestream_source::open_stream(jpx_input_box *app_box)
{
  assert(state != NULL);
  if (!state->is_stream_ready())
    return NULL;
  if (app_box == NULL)
    {
      if (!state->stream_opened)
        {
          assert(state->stream_box.exists());
          state->stream_opened = true;
          return &state->stream_box;
        }
      if (state->stream_box.exists())
        { KDU_ERROR_DEV(e,39); e <<
            KDU_TXT("Attempting to use `jpx_codestream_source::open_stream' a "
            "second time, to gain access to the same codestream, without "
            "first closing the box.  To maintain multiple open instances "
            "of the same codestream, you should supply your own "
            "`jpx_input_box' object, rather than attempting to use the "
            "internal resource multiple times concurrently.");
        }
      app_box = &(state->stream_box);
    }
  if (state->fragment_list != NULL)
    app_box->open_as(jpx_fragment_list(state->fragment_list),
                     state->owner->get_data_references(),
                     state->ultimate_src,jp2_codestream_4cc);
  else
    app_box->open(state->ultimate_src,state->stream_loc);
  return app_box;
}


/* ========================================================================= */
/*                              jx_layer_source                              */
/* ========================================================================= */

/*****************************************************************************/
/*                      jx_layer_source::donate_jplh_box                     */
/*****************************************************************************/

void
  jx_layer_source::donate_jplh_box(jp2_input_box &src)
{
  assert((!header_loc) && (!jplh) && (!sub_box));
  jplh.transplant(src);
  header_loc = jplh.get_locator();
  finish();
}

/*****************************************************************************/
/*                          jx_layer_source::finish                          */
/*****************************************************************************/

bool
  jx_layer_source::finish()
{
  if (finished)
    return true;

  // Parse top level boxes as required
  while ((!header_loc) && !owner->are_layer_header_boxes_complete())
    if (!owner->parse_next_top_level_box())
      break;

  j2_colour *cscan;

  if (jplh.exists())
    { // Parse as much as we can of the compositing layer header box
      bool jplh_complete = jplh.is_complete();
      jp2_input_box *parent_box = (cgrp.exists())?(&cgrp):(&jplh);
      while (sub_box.exists() || sub_box.open(parent_box) ||
             ((parent_box==&cgrp) && cgrp.is_complete() &&
              cgrp.close() && sub_box.open(parent_box=&jplh)))
        {
          if (sub_box.get_box_type() == jp2_colour_group_4cc)
            {
              if (cgrp.exists())
                { // Ignore nested colour group box
                  sub_box.close(); continue;
                }
              cgrp.transplant(sub_box);
              parent_box = &cgrp;
              continue;
            }
          bool sub_complete = sub_box.is_complete();
          if (sub_box.get_box_type() == jp2_colour_4cc)
            {
              if (!sub_complete)
                return false;
              if (!cgrp.exists())
                { // Colour desc boxes must be inside a colour group box
                  KDU_WARNING_DEV(w,1); w <<
                  KDU_TXT("Colour description (colr) box found "
                          "inside a compositing layer header (jplh) box, "
                          "but not wrapped by a colour group (cgrp) box.  "
                          "This is technically a violation of the JPX "
                          "standard, but we will parse the box anyway.");
                }
              for (cscan=&colour; cscan->next != NULL; cscan=cscan->next);
              if (cscan->is_initialized())
                cscan = cscan->next = new j2_colour;
              cscan->init(&sub_box);
            }
          else if (((sub_box.get_box_type()==jp2_channel_definition_4cc) ||
                    (sub_box.get_box_type() == jp2_opacity_4cc)) &&
                   (parent_box==&jplh))       
            {
              if (!sub_complete)
                return false;
              channels.init(&sub_box);
            }
          else if ((sub_box.get_box_type() == jp2_resolution_4cc) &&
                   (parent_box==&jplh))
            {
              if (!sub_complete)
                return false;
              resolution.init(&sub_box);
            }
          else if ((sub_box.get_box_type() == jp2_registration_4cc) &&
                   (parent_box==&jplh))
            {
              if (!sub_complete)
                return false;
              registration.init(&sub_box);
            }
          else if ((parent_box==&jplh) &&
                   owner->test_box_filter(sub_box.get_box_type()))
            { // Put into the metadata management system
              if (metanode == NULL)
                {
                  jx_metanode *tree = owner->get_metatree();
                  metanode = tree->add_numlist(0,NULL,1,&id,false);
                  metanode->flags |= JX_METANODE_EXISTING;
                  metanode->read_state = new jx_metaread;
                  metanode->read_state->layer_source = this;
                }
              int dbin_nest = (sub_box.get_locator().get_databin_id()==0)?1:0;
              metanode->add_descendant()->donate_input_box(sub_box,dbin_nest);
              assert(!sub_box.exists());
            }
          else
            sub_box.close(); // Discard it
        }
          
      // See if we have enough of the layer header to finish parsing it
      if (jplh_complete)
        {
          jplh.close();
          if (metanode != NULL)
            {
              metanode->flags |= JX_METANODE_DESCENDANTS_KNOWN;
              metanode->update_completed_descendants();
              delete metanode->read_state;
              metanode->read_state = NULL;
            }
        }
      else if (colour.is_initialized() && (!cgrp) &&
               channels.is_initialized() && resolution.is_initialized() &&
               registration.is_initialized())
        { // Donate `jplh' to the metadata management machinery to extract
          // any remaining boxes if it can.
          if (metanode == NULL)
            {
              jx_metanode *tree = owner->get_metatree();
              metanode = tree->add_numlist(0,NULL,1,&id,false);
              metanode->flags |= JX_METANODE_EXISTING;
              metanode->read_state = new jx_metaread;
            }
          metanode->read_state->layer_source = NULL;
          metanode->read_state->asoc.transplant(jplh);
          assert(!jplh);
        }
    }

  if (jplh.exists() ||
      ((!header_loc) && !owner->are_layer_header_boxes_complete()))
    return false; // Still waiting for the compositing layer header box

  // Check that all required codestreams are available
  registration.finalize(id); // Safe to do this multiple times
  int cs_id, cs_which;
  for (cs_which=0; cs_which < registration.num_codestreams; cs_which++)
    {
      cs_id = registration.codestreams[cs_which].codestream_id;
      jx_codestream_source *cs = owner->get_codestream(cs_id);
      if ((cs == NULL) && owner->is_top_level_complete())
        { KDU_ERROR(e,40); e <<
            KDU_TXT("Encountered a JPX compositing layer box which "
            "utilizes a non-existent codestream!");
        }
      if ((cs == NULL) || !cs->finish(true))
        return false;
    }

  // See if we need to evaluate the layer dimensions
  if ((registration.final_layer_size.x == 0) ||
      (registration.final_layer_size.y == 0))
    {
      registration.final_layer_size = kdu_coords(0,0);
      for (cs_which=0; cs_which < registration.num_codestreams; cs_which++)
        {
          cs_id = registration.codestreams[cs_which].codestream_id;
          jx_codestream_source *cs = owner->get_codestream(cs_id);
          jpx_codestream_source cs_ifc(cs);
          kdu_coords size = cs_ifc.access_dimensions().get_size();
          size.x *= registration.codestreams[cs_which].sampling.x;
          size.y *= registration.codestreams[cs_which].sampling.y;
          size += registration.codestreams[cs_which].alignment;
          if ((cs_which == 0) || (size.x < registration.final_layer_size.x))
            registration.final_layer_size.x = size.x;
          if ((cs_which == 0) || (size.y < registration.final_layer_size.y))
            registration.final_layer_size.y = size.y;
        }
      registration.final_layer_size.x =
        ceil_ratio(registration.final_layer_size.x,registration.denominator.x);
      registration.final_layer_size.y =
        ceil_ratio(registration.final_layer_size.y,registration.denominator.y);
    }

  // Collect defaults, as required
  if (!colour.is_initialized())
    { // Colour description must be in the default header box
      j2_colour *dflt = owner->get_default_colour();
      if (dflt == NULL)
        return false;
      for (cscan=&colour;
           (dflt != NULL) && dflt->is_initialized();
           dflt=dflt->next)
        {
          if (cscan->is_initialized())
            cscan = cscan->next = new j2_colour;
          cscan->copy(dflt);
        }
    }

  if (!channels.is_initialized())
    { // Channel defs information might be in default header box
      j2_channels *dflt = owner->get_default_channels();
      if (dflt == NULL)
        return false;
      if (dflt->is_initialized())
        channels.copy(dflt);
    }
  
  if (!resolution.is_initialized())
    { // Resolution information might be in default header box
      j2_resolution *dflt = owner->get_default_resolution();
      if (dflt == NULL)
        return false;
      if (dflt->is_initialized())
        resolution.copy(dflt);
    }

  // Finalize boxes
  int num_colours = 0;
  for (cscan=&colour; (cscan != NULL) && (num_colours == 0); cscan=cscan->next)
    num_colours = cscan->get_num_colours();
  channels.finalize(num_colours,false);
  for (cs_which=0; cs_which < registration.num_codestreams; cs_which++)
    {
      cs_id = registration.codestreams[cs_which].codestream_id;
      jx_codestream_source *cs = owner->get_codestream(cs_id);
      assert(cs != NULL);
      channels.find_cmap_channels(cs->get_component_map(),cs_id);
    }
  if (!channels.all_cmap_channels_found())
    { KDU_ERROR(e,41); e <<
        KDU_TXT("JP2/JPX source is internally inconsistent.  "
        "Either an explicit channel mapping box, or the set of channels "
        "implicitly identified by a colour space box, cannot all be "
        "associated with available code-stream image components.");
    }
  for (cscan=&colour; cscan != NULL; cscan=cscan->next)
    cscan->finalize(&channels);
  finished = true;
  return true;
}

/*****************************************************************************/
/*                    jx_layer_source::check_stream_headers                  */
/*****************************************************************************/

bool
  jx_layer_source::check_stream_headers()
{
  if (!finished)
    return false;
  int cs_id, cs_which;
  for (cs_which=0; cs_which < registration.num_codestreams; cs_which++)
    {
      cs_id = registration.codestreams[cs_which].codestream_id;
      jx_codestream_source *cs = owner->get_codestream(cs_id);
      assert(cs != NULL);
      if (!cs->is_stream_ready())
        return false;
    }
  stream_headers_available = true;
  return true;
}


/* ========================================================================= */
/*                             jpx_layer_source                              */
/* ========================================================================= */

/*****************************************************************************/
/*                       jpx_layer_source::get_layer_id                      */
/*****************************************************************************/

int
  jpx_layer_source::get_layer_id()
{
  assert((state != NULL) && state->finished);
  return state->id;
}

/*****************************************************************************/
/*                      jpx_layer_source::get_header_loc                     */
/*****************************************************************************/

jp2_locator
  jpx_layer_source::get_header_loc()
{
  assert((state != NULL) && state->finished);
  return state->header_loc;
}

/*****************************************************************************/
/*                     jpx_layer_source::access_channels                     */
/*****************************************************************************/

jp2_channels
  jpx_layer_source::access_channels()
{
  assert((state != NULL) && state->finished);
  return jp2_channels(&state->channels);
}

/*****************************************************************************/
/*                    jpx_layer_source::access_resolution                    */
/*****************************************************************************/

jp2_resolution
  jpx_layer_source::access_resolution()
{
  assert((state != NULL) && state->finished);
  return jp2_resolution(&state->resolution);
}

/*****************************************************************************/
/*                      jpx_layer_source::access_colour                      */
/*****************************************************************************/

jp2_colour
  jpx_layer_source::access_colour(int which)
{
  assert((state != NULL) && state->finished);
  j2_colour *scan=&state->colour;
  for (; (which > 0) && (scan != NULL); which--)
    scan = scan->next;
  return jp2_colour(scan);
}

/*****************************************************************************/
/*                   jpx_layer_source::get_num_codestreams                   */
/*****************************************************************************/

int
  jpx_layer_source::get_num_codestreams()
{
  assert((state != NULL) && state->finished);
  return state->registration.num_codestreams;
}

/*****************************************************************************/
/*                    jpx_layer_source::get_codestream_id                    */
/*****************************************************************************/

int
  jpx_layer_source::get_codestream_id(int which)
{
  assert((state != NULL) && state->finished);
  if ((which < 0) || (which >= state->registration.num_codestreams))
    return -1;
  return state->registration.codestreams[which].codestream_id;
}

/*****************************************************************************/
/*                   jpx_layer_source::have_stream_headers                   */
/*****************************************************************************/

bool
  jpx_layer_source::have_stream_headers()
{
  assert(state != NULL);
  return state->all_streams_ready();
}

/*****************************************************************************/
/*                    jpx_layer_source::get_layer_size                       */
/*****************************************************************************/

kdu_coords
  jpx_layer_source::get_layer_size()
{
  assert((state != NULL) && state->finished);
  return state->registration.final_layer_size;
}

/*****************************************************************************/
/*               jpx_layer_source::get_codestream_registration               */
/*****************************************************************************/

int
  jpx_layer_source::get_codestream_registration(int which,
                                                kdu_coords &alignment,
                                                kdu_coords &sampling,
                                                kdu_coords &denominator)
{
  assert((state != NULL) && state->finished);
  denominator = state->registration.denominator;
  if ((which < 0) || (which >= state->registration.num_codestreams))
    return -1;
  alignment = state->registration.codestreams[which].alignment;
  sampling = state->registration.codestreams[which].sampling;
  return state->registration.codestreams[which].codestream_id;
}


/* ========================================================================= */
/*                                 jx_source                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                            jx_source::jx_source                           */
/*****************************************************************************/

jx_source::jx_source(jp2_family_src *src)
{
  ultimate_src = src;
  ultimate_src_id = src->get_id();
  have_signature = false;
  have_file_type = false;
  have_reader_requirements = false;
  is_completely_open = false;
  restrict_to_jp2 = false;
  in_parse_next_top_level_box = false;
  num_codestreams = 0;
  num_layers = 0;
  top_level_complete = false;
  jp2h_box_found = jp2h_box_complete = false;
  have_dtbl_box = false;
  max_codestreams = max_layers = 0;
  codestreams = NULL;
  layers = NULL;
  num_jp2c_or_ftbl_encountered = 0;
  num_chdr_encountered = 0;
  meta_manager.source = this;
  meta_manager.ultimate_src = src;
  meta_manager.tree = new jx_metanode(&meta_manager);
  meta_manager.tree->flags |= JX_METANODE_EXISTING;
}

/*****************************************************************************/
/*                           jx_source::~jx_source                           */
/*****************************************************************************/

jx_source::~jx_source()
{
  j2_colour *cscan;

  int n;
  for (n=0; n < num_codestreams; n++)
    if (codestreams[n] != NULL)
      { delete codestreams[n]; codestreams[n] = NULL; }
  if (codestreams != NULL)
    delete[] codestreams;
  for (n=0; n < num_layers; n++)
    if (layers[n] != NULL)
      { delete layers[n]; layers[n] = NULL; }
  if (layers != NULL)
    delete[] layers;
  while ((cscan=default_colour.next) != NULL)
    { default_colour.next = cscan->next; delete cscan; }
}

/*****************************************************************************/
/*                     jx_source::parse_next_top_level_box                   */
/*****************************************************************************/

bool
  jx_source::parse_next_top_level_box(bool already_open)
{
  if (top_level_complete || in_parse_next_top_level_box)
    {
      assert(!already_open);
      return false;
    }
  if (!already_open)
    {
      assert(!top_box);
      if (!top_box.open_next())
        {
          if (!ultimate_src->is_top_level_complete())
            return false;

          if (!top_box.open_next()) // Try again, to avoid race conditions
            {
              top_level_complete = true;

              if (num_layers == 0)
                { // We encountered no compositing layer header boxes,
                  // so there must be one layer per codestream
                  while (num_layers < num_codestreams)
                    add_layer();
                }
              return false;
            }
        }
    }

  in_parse_next_top_level_box = true;
  if (top_box.get_box_type() == jp2_dtbl_4cc)
    {
      if (have_dtbl_box)
        { KDU_ERROR(e,42); e <<
          KDU_TXT("JP2-family data source appears to contain "
                  "more than one data reference (dtbl) box.  At most "
                  "one should be found in the file.");
        }
      have_dtbl_box = true;
      dtbl_box.transplant(top_box);
      if (dtbl_box.is_complete())
        {
          data_references.init(&dtbl_box);
          assert(!dtbl_box);
        }
    }
  else if (top_box.get_box_type() == jp2_header_4cc)
    {
      if (jp2h_box_found)
        { KDU_ERROR(e,43); e <<
          KDU_TXT("JP2-family data source contains more than one "
                  "top-level JP2 header (jp2h) box.");
        }
      jp2h_box_found = true;
      jp2h_box.transplant(top_box);
      finish_jp2_header_box();
    }
  else if ((top_box.get_box_type() == jp2_codestream_4cc) ||
           (top_box.get_box_type() == jp2_fragment_table_4cc))
    {
      if (num_jp2c_or_ftbl_encountered == num_codestreams)
        add_codestream(); // We have found a new codestream
      jx_codestream_source *cs=codestreams[num_jp2c_or_ftbl_encountered++];
      cs->donate_codestream_box(top_box);
      assert(!top_box);
    }
  else if (top_box.get_box_type() == jp2_codestream_header_4cc)
    {
      if (num_chdr_encountered == num_codestreams)
        add_codestream(); // We have found a new codestream
      jx_codestream_source *cs=codestreams[num_chdr_encountered++];
      cs->donate_chdr_box(top_box);
      assert(!top_box);
    }
  else if ((top_box.get_box_type() == jp2_compositing_layer_hdr_4cc) &&
           !restrict_to_jp2)
    {
      jx_layer_source *lyr = add_layer();
      lyr->donate_jplh_box(top_box);
      assert(!top_box);
    }
  else if (top_box.get_box_type() == jp2_composition_4cc)
    {
      composition.donate_composition_box(top_box,this);
      assert(!top_box);
    }
  else if (meta_manager.test_box_filter(top_box.get_box_type()))
    { // Build into meta-data structure
      jx_metanode *node = meta_manager.tree->add_descendant();
      node->donate_input_box(top_box,0);
      assert(!top_box);
    }
  else
    top_box.close();
  in_parse_next_top_level_box = false;

  if (restrict_to_jp2 && (num_layers == 0) && (num_codestreams > 0))
    { // JP2 files have one layer, corresponding to the 1st and only
      // codestream
      assert(layers == NULL);
      add_layer();
    }
  
  return true;
}

/*****************************************************************************/
/*                      jx_source::finish_jp2_header_box                     */
/*****************************************************************************/

bool
  jx_source::finish_jp2_header_box()
{
  if (jp2h_box_complete)
    return true;
  while ((!jp2h_box_found) && !is_top_level_complete())
    if (!parse_next_top_level_box())
      break;
  if (!jp2h_box_found)
    return top_level_complete;
  
  bool result = false;
  assert(jp2h_box.exists());
  while (sub_box.exists() || sub_box.open(&jp2h_box))
    {
      bool sub_complete = sub_box.is_complete();
      if (sub_box.get_box_type() == jp2_image_header_4cc)
        {
          if (!sub_complete)
            break;
          default_dimensions.init(&sub_box);
        }
      else if (sub_box.get_box_type() == jp2_bits_per_component_4cc)
        {
          if (!sub_complete)
            break;
          default_dimensions.process_bpcc_box(&sub_box);
          have_default_bpcc = true;
        }
      else if (sub_box.get_box_type() == jp2_palette_4cc)
        {
          if (!sub_complete)
            break;
          default_palette.init(&sub_box);
        }
      else if (sub_box.get_box_type() == jp2_component_mapping_4cc)
        {
          if (!sub_complete)
            break;
          default_component_map.init(&sub_box);
        }
      else if (sub_box.get_box_type() == jp2_colour_4cc)
        {
          if (!sub_complete)
            break;
          j2_colour *cscan;
          for (cscan=&default_colour; cscan->next != NULL;
               cscan=cscan->next);
          if (cscan->is_initialized())
            cscan = cscan->next = new j2_colour;
          cscan->init(&sub_box);
        }
      else if (sub_box.get_box_type() == jp2_channel_definition_4cc)
        {
          if (!sub_complete)
            break;
          default_channels.init(&sub_box);
        }
      else if (sub_box.get_box_type() == jp2_resolution_4cc)
        {
          if (!sub_complete)
            break;
          default_resolution.init(&sub_box);
        }
      else
        sub_box.close(); // Skip over other boxes.
    }
  
  // See if we have enough of the JP2 header to finish parsing it
  if ((!sub_box) &&
      (jp2h_box.is_complete() ||
       (default_dimensions.is_initialized() && have_default_bpcc &&
        default_palette.is_initialized() &&
        default_component_map.is_initialized() &&
        default_channels.is_initialized() &&
        default_colour.is_initialized() &&
        default_resolution.is_initialized())))
    {
      jp2h_box.close();
      default_resolution.finalize();
      jp2h_box_complete = true;
      result = true;
    }

  return result;
}

/*****************************************************************************/
/*                           jx_source::add_codestream                       */
/*****************************************************************************/

jx_codestream_source *jx_source::add_codestream()
{
  if (max_codestreams <= num_codestreams)
    {
      int new_max_cs = max_codestreams*2 + 1;
      jx_codestream_source **buf = new jx_codestream_source *[new_max_cs];
      memset(buf,0,sizeof(jx_codestream_source *)*(size_t)new_max_cs);
      if (codestreams != NULL)
        {
          memcpy(buf,codestreams,
                 sizeof(jx_codestream_source *)*(size_t)num_codestreams);
          delete[] codestreams;
        }
      codestreams = buf;
      max_codestreams = new_max_cs;
    }
  jx_codestream_source *result = codestreams[num_codestreams] =
    new jx_codestream_source(this,ultimate_src,num_codestreams,
                             restrict_to_jp2);
  num_codestreams++;
  return result;
}

/*****************************************************************************/
/*                             jx_source::add_layer                          */
/*****************************************************************************/

jx_layer_source *jx_source::add_layer()
{
  if (max_layers <= num_layers)
    {
      int new_max_layers = max_layers*2 + 1;
      jx_layer_source **buf = new jx_layer_source *[new_max_layers];
      memset(buf,0,sizeof(jx_layer_source *)*(size_t)new_max_layers);
      if (layers != NULL)
        {
          memcpy(buf,layers,sizeof(jx_layer_source *)*(size_t)num_layers);
          delete[] layers;
        }
      layers = buf;
      max_layers = new_max_layers;
    }
  jx_layer_source *result = layers[num_layers] =
    new jx_layer_source(this,num_layers);
  num_layers++;
  return result;
}


/* ========================================================================= */
/*                                 jpx_source                                */
/* ========================================================================= */

/*****************************************************************************/
/*                           jpx_source::jpx_source                          */
/*****************************************************************************/

jpx_source::jpx_source()
{
  state = NULL;
}

/*****************************************************************************/
/*                              jpx_source::open                             */
/*****************************************************************************/

int
  jpx_source::open(jp2_family_src *src, bool return_if_incompatible)
{
  if (state == NULL)
    state = new jx_source(src);
  if (state->is_completely_open)
    { KDU_ERROR_DEV(e,44); e <<
        KDU_TXT("Attempting invoke `jpx_source::open' on a JPX "
        "source object which has been completely opened, but not yet closed.");
    }
  if ((state->ultimate_src != src) ||
      (src->get_id() != state->ultimate_src_id))
    { // Treat as though it was closed
      delete state;
      state = new jx_source(src);
    }

  // First, check for a signature box
  if (!state->have_signature)
    {
      if (!((state->top_box.exists() || state->top_box.open(src)) &&
            state->top_box.is_complete()))
        {
          if (src->uses_cache())
            return 0;
          else
            {
              close();
              if (return_if_incompatible)
                return -1;
              else
                { KDU_ERROR(e,45); e <<
                    KDU_TXT("Data source supplied to `jpx_source::open' "
                    "does not commence with a valid JP2-family signature "
                    "box.");
                }
            }
        }
      kdu_uint32 signature;
      if ((state->top_box.get_box_type() != jp2_signature_4cc) ||
          !(state->top_box.read(signature) && (signature == jp2_signature) &&
            (state->top_box.get_remaining_bytes() == 0)))
        {
          close();
          if (return_if_incompatible)
            return -1;
          else
            { KDU_ERROR(e,46); e <<
                KDU_TXT("Data source supplied to `jpx_source::open' "
                "does not commence with a valid JP2-family signature box.");
            }
        }
      state->top_box.close();
      state->have_signature = true;
    }

  // Next, check for a file-type box
  if (!state->have_file_type)
    {
      if (!((state->top_box.exists() || state->top_box.open_next()) &&
            state->top_box.is_complete()))
        {
          if (src->uses_cache())
            return 0;
          else
            {
              close();
              if (return_if_incompatible)
                return -1;
              else
                { KDU_ERROR(e,47);  e <<
                    KDU_TXT("Data source supplied to `jpx_source::open' "
                    "does not contain a correctly positioned file-type "
                    "(ftyp) box.");
                }
            }
        }
      if (state->top_box.get_box_type() != jp2_file_type_4cc)
        {
          close();
          if (return_if_incompatible)
            return -1;
          else
            { KDU_ERROR(e,48); e <<
                KDU_TXT("Data source supplied to `jpx_source::open' "
                "does not contain a correctly positioned file-type "
                "(ftyp) box.");
            }
        }
      if (!state->compatibility.init_ftyp(&state->top_box))
        { // Not compatible with JP2 or JPX.
          close();
          if (return_if_incompatible)
            return -1;
          else
            { KDU_ERROR(e,49); e <<
                KDU_TXT("Data source supplied to `jpx_source::open' "
                "contains a correctly positioned file-type box, but that box "
                "does not identify either JP2 or JPX as a compatible file "
                "type.");
            }
        }
      assert(!state->top_box);
      state->have_file_type = true;
      jpx_compatibility compat(&state->compatibility);
      state->restrict_to_jp2 = compat.is_jp2();
    }

  if (state->restrict_to_jp2)
    { // Ignore reader requirements box, if any
      state->is_completely_open = true;
      return 1;
    }

  // Need to check for a reader requirements box
  assert(!state->have_reader_requirements);
  if (!((state->top_box.exists() || state->top_box.open_next()) &&
        ((state->top_box.get_box_type() != jp2_reader_requirements_4cc) ||
         state->top_box.is_complete())))
    {
      if (src->uses_cache())
        return 0;
      else
        {
          close();
          if (return_if_incompatible)
            return -1;
          else
            { KDU_ERROR(e,50); e <<
                KDU_TXT("Data source supplied to `jpx_source::open' "
                "does not contain a correctly positioned reader "
                "requirements box.");
            }
        }
    }
  if (state->top_box.get_box_type() == jp2_reader_requirements_4cc)
    { // Allow the reader requirements box to be missing, even though it
      // is not strictly legal
      state->compatibility.init_rreq(&state->top_box);
      state->have_reader_requirements = true;
      assert(!state->top_box);
    }
  else
    state->parse_next_top_level_box(true);

  // If we get here, we have done everything
  state->is_completely_open = true;
  return 1;
}

/*****************************************************************************/
/*                        jpx_source::get_ultimate_src                      */
/*****************************************************************************/

jp2_family_src *
  jpx_source::get_ultimate_src()
{
  if (state == NULL)
    return NULL;
  else
    return state->ultimate_src;
}

/*****************************************************************************/
/*                              jpx_source::close                            */
/*****************************************************************************/

bool
  jpx_source::close()
{
  if (state == NULL)
    return false;
  bool result = state->is_completely_open;
  delete state;
  state = NULL;
  return result;
}

/*****************************************************************************/
/*                     jpx_source::access_data_references                    */
/*****************************************************************************/

jp2_data_references
  jpx_source::access_data_references()
{
  if ((state == NULL) || !state->is_completely_open)
    return jp2_data_references(NULL);
  while ((!state->have_dtbl_box) && (!state->top_level_complete) &&
         state->parse_next_top_level_box());
  if (state->dtbl_box.exists() && state->dtbl_box.is_complete())
    {
      state->data_references.init(&state->dtbl_box);
      assert(!(state->dtbl_box));
    }
  if (state->have_dtbl_box || state->top_level_complete)
    return jp2_data_references(&(state->data_references));
  else
    return jp2_data_references(NULL);
}

/*****************************************************************************/
/*                      jpx_source::access_compatibility                     */
/*****************************************************************************/

jpx_compatibility
  jpx_source::access_compatibility()
{
  jpx_compatibility result;
  if ((state != NULL) & state->is_completely_open)
    result = jpx_compatibility(&state->compatibility);
  return result;
}

/*****************************************************************************/
/*                        jpx_source::count_codestreams                      */
/*****************************************************************************/

bool
  jpx_source::count_codestreams(int &count)
{
  if ((state == NULL) || !state->is_completely_open)
    { count = 0; return false; }
  while ((!state->top_level_complete) &&
         state->parse_next_top_level_box());
  count = state->num_codestreams;
  return state->top_level_complete;
}

/*****************************************************************************/
/*                   jpx_source::count_compositing_layers                    */
/*****************************************************************************/

bool
  jpx_source::count_compositing_layers(int &count)
{
  if ((state == NULL) || !state->is_completely_open)
    { count = 0; return false; }
  if (!state->restrict_to_jp2)
    while ((!state->top_level_complete) && state->parse_next_top_level_box());
  count = state->num_layers;
  if ((count < 1) && state->restrict_to_jp2)
    count = 1; // We always have at least one, even if we have to fake it
  return (state->top_level_complete || state->restrict_to_jp2);
}

/*****************************************************************************/
/*                       jpx_source::access_codestream                       */
/*****************************************************************************/

jpx_codestream_source
  jpx_source::access_codestream(int which, bool need_main_header)
{
  jpx_codestream_source result;
  if ((state == NULL) || (!state->is_completely_open) || (which < 0))
    return result; // Return an empty interface
  while ((state->num_codestreams <= which) && (!state->top_level_complete))
    if (!state->parse_next_top_level_box())
      {
        if ((which == 0) && state->top_level_complete)
          { KDU_ERROR(e,51); e <<
              KDU_TXT("JPX data source appears to contain no "
              "codestreams at all.");
          }
        return result; // Return an empty interface
      }
  jx_codestream_source *cs = state->codestreams[which];
  if (cs->finish(true))
    {
      if ((!need_main_header) || cs->is_stream_ready())
        result = jpx_codestream_source(cs);
    }
  else if (state->top_level_complete && !cs->have_all_boxes())
    { KDU_ERROR(e,52); e <<
        KDU_TXT("JPX data source appears to contain an incomplete "
        "codestream.  Specifically, this either means that an embedded "
        "contiguous or fragmented codestream has been found without "
        "sufficient descriptive metadata, or that a codestream header box "
        "has been found without any matching embedded contiguous or "
        "fragmented codestream.  Both of these conditions are illegal for "
        "JPX and JP2 data sources.");
    }
  return result;
}

/*****************************************************************************/
/*                          jpx_source::access_layer                         */
/*****************************************************************************/

jpx_layer_source
  jpx_source::access_layer(int which, bool need_stream_headers)
{
  jpx_layer_source result;
  if ((state == NULL) || (!state->is_completely_open) || (which < 0))
    return result; // Return an empty interface
  if (state->restrict_to_jp2 && (which != 0))
    return result; // Return empty interface

  while ((state->num_layers <= which) && (!state->top_level_complete))
    if (!state->parse_next_top_level_box())
      break;
  if (state->num_layers <= which)
    return result; // Return an empty interface

  jx_layer_source *layer = state->layers[which];
  if (layer->finish())
    {
      if ((!need_stream_headers) || layer->all_streams_ready())
        result = jpx_layer_source(layer);
    }
  return result;
}

/*****************************************************************************/
/*                    jpx_source::get_num_layer_codestreams                  */
/*****************************************************************************/

int
  jpx_source::get_num_layer_codestreams(int which_layer)
{
  if ((state == NULL) || (!state->is_completely_open) || (which_layer < 0))
    return 0;
  if (state->restrict_to_jp2 && (which_layer != 0))
    return 0;

  while ((state->num_layers <= which_layer) && (!state->top_level_complete))
    if (!state->parse_next_top_level_box())
      break;
  if (state->num_layers <= which_layer)
    return 0;

  jx_layer_source *layer = state->layers[which_layer];
  layer->finish(); // Try to finish it

  jx_registration *reg = layer->access_registration();
  return reg->num_codestreams;
}

/*****************************************************************************/
/*                     jpx_source::get_layer_codestream_id                   */
/*****************************************************************************/

int
  jpx_source::get_layer_codestream_id(int which_layer, int which_stream)
{
  if ((state == NULL) || (!state->is_completely_open) ||
      (which_layer < 0) || (which_stream < 0))
    return -1;
  if (state->restrict_to_jp2 && (which_layer != 0))
    return -1;

  while ((state->num_layers <= which_layer) && (!state->top_level_complete))
    if (!state->parse_next_top_level_box())
      break;
  if (state->num_layers <= which_layer)
    return -1;

  jx_layer_source *layer = state->layers[which_layer];
  layer->finish(); // Try to finish it

  jx_registration *reg = layer->access_registration();
  if (which_stream >= reg->num_codestreams)
    return -1;
  return reg->codestreams[which_stream].codestream_id;
}

/*****************************************************************************/
/*                       jpx_source::access_composition                      */
/*****************************************************************************/

jpx_composition
  jpx_source::access_composition()
{
  jpx_composition result;
  if ((state != NULL) && state->composition.finish(state))
    result = jpx_composition(&state->composition);
  return result;
}

/*****************************************************************************/
/*                      jpx_source::access_meta_manager                      */
/*****************************************************************************/

jpx_meta_manager
  jpx_source::access_meta_manager()
{
  jpx_meta_manager result;
  if (state != NULL)
    result = jpx_meta_manager(&state->meta_manager);
  return result;
}


/* ========================================================================= */
/*                           jx_codestream_target                            */
/* ========================================================================= */

/*****************************************************************************/
/*                  jpx_codestream_target::get_codestream_id                 */
/*****************************************************************************/

int
  jpx_codestream_target::get_codestream_id()
{
  assert(state != NULL);
  return state->id;
}

/*****************************************************************************/
/*                      jx_codestream_target::finalize                       */
/*****************************************************************************/

void
  jx_codestream_target::finalize()
{
  if (finalized)
    return;
  dimensions.finalize();
  palette.finalize();
  component_map.finalize(&dimensions,&palette);
  finalized = true;
}

/*****************************************************************************/
/*                     jx_codestream_target::write_chdr                      */
/*****************************************************************************/

jp2_output_box *
  jx_codestream_target::write_chdr(int *i_param, void **addr_param)
{
  assert(finalized && !chdr_written);
  if (chdr.exists())
    { // We must have been processing a breakpoint.
      assert(have_breakpoint);
      chdr.close();
      chdr_written = true;
      return NULL;
    }
  owner->open_top_box(&chdr,jp2_codestream_header_4cc);
  if (!owner->get_default_dimensions()->compare(&dimensions))
    dimensions.save_boxes(&chdr);
  j2_palette *def_palette = owner->get_default_palette();
  j2_component_map *def_cmap = owner->get_default_component_map();
  if (def_palette->is_initialized())
    { // Otherwise, there will be no default component map either
      if (!def_palette->compare(&palette))
        palette.save_box(&chdr);
      if (!def_cmap->compare(&component_map))
        component_map.save_box(&chdr,true); // Force generation of a cmap box
    }
  else
    {
      palette.save_box(&chdr);
      component_map.save_box(&chdr);
    }
  if (have_breakpoint)
    {
      if (i_param != NULL)
        *i_param = this->i_param;
      if (addr_param != NULL)
        *addr_param = this->addr_param;
      return &chdr;
    }
  chdr.close();
  chdr_written = true;
  return NULL;
}

/*****************************************************************************/
/*                    jx_codestream_target::copy_defaults                    */
/*****************************************************************************/

void
  jx_codestream_target::copy_defaults(j2_dimensions &default_dims,
                                      j2_palette &default_plt,
                                      j2_component_map &default_map)
{
  default_dims.copy(&dimensions);
  default_plt.copy(&palette);
  default_map.copy(&component_map);
}

/*****************************************************************************/
/*               jx_codestream_target::adjust_compatibility                  */
/*****************************************************************************/

void
  jx_codestream_target::adjust_compatibility(jx_compatibility *compatibility)
{
  assert(finalized);
  int profile, compression_type;
  compression_type = dimensions.get_compression_type(profile);
  if (compression_type == JP2_COMPRESSION_TYPE_JPEG2000)
    {
      if (profile < 0)
        profile = Sprofile_PROFILE2; // Reasonable guess, if never initialized
      if (profile == Sprofile_PROFILE0)
        compatibility->add_standard_feature(JPX_SF_JPEG2000_PART1_PROFILE0);
      else if (profile == Sprofile_PROFILE1)
        compatibility->add_standard_feature(JPX_SF_JPEG2000_PART1_PROFILE1);
      else if ((profile == Sprofile_PROFILE2) ||
               (profile = Sprofile_CINEMA2K) ||
               (profile == Sprofile_CINEMA4K))
        compatibility->add_standard_feature(JPX_SF_JPEG2000_PART1);
      else
        {
          compatibility->add_standard_feature(JPX_SF_JPEG2000_PART2);
          compatibility->have_non_part1_codestream();
          if (id == 0)
            {
              compatibility->set_not_jp2_compatible();
              if (!dimensions.get_jpxb_compatible())
                compatibility->set_not_jpxb_compatible();
            }
        }
    }
  else
    {
      if (id == 0)
        {
          compatibility->set_not_jp2_compatible();
          compatibility->set_not_jpxb_compatible();
        }
      compatibility->have_non_part1_codestream();
      if (compression_type == JP2_COMPRESSION_TYPE_JPEG)
        compatibility->add_standard_feature(JPX_SF_CODESTREAM_USING_DCT);
    }
}


/* ========================================================================= */
/*                          jpx_codestream_target                            */
/* ========================================================================= */

/*****************************************************************************/
/*                  jpx_codestream_target::access_dimensions                 */
/*****************************************************************************/

jp2_dimensions
  jpx_codestream_target::access_dimensions()
{
  assert(state != NULL);
  return jp2_dimensions(&state->dimensions);
}

/*****************************************************************************/
/*                   jpx_codestream_target::access_palette                   */
/*****************************************************************************/

jp2_palette
  jpx_codestream_target::access_palette()
{
  assert(state != NULL);
  return jp2_palette(&state->palette);
}

/*****************************************************************************/
/*                    jpx_codestream_target::set_breakpoint                  */
/*****************************************************************************/

void
  jpx_codestream_target::set_breakpoint(int i_param, void *addr_param)
{
  assert(state != NULL);
  state->have_breakpoint = true;
  state->i_param = i_param;
  state->addr_param = addr_param;
}

/*****************************************************************************/
/*                 jpx_codestream_target::access_fragment_list               */
/*****************************************************************************/

jpx_fragment_list
  jpx_codestream_target::access_fragment_list()
{
  return jpx_fragment_list(&(state->fragment_list));
}

/*****************************************************************************/
/*                     jpx_codestream_target::add_fragment                   */
/*****************************************************************************/

void
  jpx_codestream_target::add_fragment(const char *url_or_path, kdu_long offset,
                                      kdu_long length, bool is_path)
{
  int url_idx;
  if (is_path && (url_or_path != NULL))
    url_idx = state->owner->get_data_references().add_file_url(url_or_path);
  else
    url_idx = state->owner->get_data_references().add_url(url_or_path);
  access_fragment_list().add_fragment(url_idx,offset,length);
}

/*****************************************************************************/
/*                 jpx_codestream_target::write_fragment_table               */
/*****************************************************************************/

void
  jpx_codestream_target::write_fragment_table()
{
  if (!state->owner->can_write_codestreams())
    { KDU_ERROR_DEV(e,53); e <<
        KDU_TXT("You may not call "
        "`jpx_codestream_target::open_stream' or "
        "`jpx_codestream_target::write_fragment_table' until after the JPX "
        "file header has been written using `jpx_target::write_headers'.");
    }
  if (state->codestream_opened)
    { KDU_ERROR_DEV(e,54); e <<
        KDU_TXT("You may not call "
        "`jpx_codestream_target::open_stream' or "
        "`jpx_codestream_target::write_fragment_table' multiple times for the "
        "same code-stream.");
    }
  if (state->fragment_list.is_empty())
    { KDU_ERROR_DEV(e,55); e <<
        KDU_TXT("You must not call "
        "`jpx_codestream_target::write_fragment_table' without first adding "
        "one or more references to codestream fragments.");
    }
  state->fragment_list.finalize(state->owner->get_data_references());
  state->owner->open_top_box(&state->jp2c,jp2_fragment_table_4cc);
  state->fragment_list.save_box(&(state->jp2c));
  state->jp2c.close();
  state->codestream_opened = true;
}

/*****************************************************************************/
/*                     jpx_codestream_target::open_stream                    */
/*****************************************************************************/

jp2_output_box *
  jpx_codestream_target::open_stream()
{
  assert(state != NULL);
  if (!state->owner->can_write_codestreams())
    { KDU_ERROR_DEV(e,56); e <<
        KDU_TXT("You may not call "
        "`jpx_codestream_target::open_stream' or "
        "`jpx_codestream_target::write_fragment_table' until after the JPX "
        "file header has been written using `jpx_target::write_headers'.");
    }
  if (!state->fragment_list.is_empty())
    { KDU_ERROR_DEV(e,57); e <<
        KDU_TXT("You may not call `open_stream' "
        "on a `jpx_codestream_target' object to which one or more "
        "codestream fragment references have already been added.  Each "
        "codestream must be represented by exactly one contiguous codestream "
        "or one fragment table, but not both.");
    }
  if (state->codestream_opened)
    { KDU_ERROR_DEV(e,58); e <<
        KDU_TXT("You may not call "
        "`jpx_codestream_target::open_stream' or "
        "`jpx_codestream_target::write_fragment_table' multiple times for the "
        "same code-stream.");
    }
  state->owner->open_top_box(&state->jp2c,jp2_codestream_4cc);
  state->codestream_opened = true;
  return &state->jp2c;
}

/*****************************************************************************/
/*                    jpx_codestream_target::access_stream                   */
/*****************************************************************************/

kdu_compressed_target *
  jpx_codestream_target::access_stream()
{
  assert(state != NULL);
  return &state->jp2c;
}


/* ========================================================================= */
/*                             jx_layer_target                               */
/* ========================================================================= */

/*****************************************************************************/
/*                        jx_layer_target::finalize                          */
/*****************************************************************************/

bool
  jx_layer_target::finalize()
{
  if (finalized)
    return need_creg;
  resolution.finalize();
  j2_colour *cscan;
  int num_colours = 0;
  for (cscan=&colour; cscan != NULL; cscan=cscan->next)
    if (num_colours == 0)
      num_colours = cscan->get_num_colours();
    else if ((num_colours != cscan->get_num_colours()) &&
             (cscan->get_num_colours() != 0))
      { KDU_ERROR_DEV(e,59); e <<
          KDU_TXT("The `jpx_layer_target::add_colour' function has "
          "been used to add multiple colour descriptions for a JPX "
          "compositing layer, but not all colour descriptions have the "
          "same number of colour channels.");
      }
  channels.finalize(num_colours,true);

  registration.finalize(&channels,this->id);
  need_creg = false; // Until proven otherwise
  int cs_id, cs_which;
  for (cs_which=0; cs_which < registration.num_codestreams; cs_which++)
    {
      cs_id = registration.codestreams[cs_which].codestream_id;
      if (cs_id != this->id)
        need_creg = true;
      jx_codestream_target *cs = owner->get_codestream(cs_id);
      if (cs == NULL)
        { KDU_ERROR_DEV(e,60); e <<
            KDU_TXT("Application has configured a JPX compositing "
            "layer box which utilizes a non-existent codestream!");
        }
      channels.add_cmap_channels(cs->get_component_map(),cs_id);
      jpx_codestream_target cs_ifc(cs);
      kdu_coords size = cs_ifc.access_dimensions().get_size();
      if ((registration.codestreams[cs_which].sampling !=
           registration.denominator) ||
          (registration.codestreams[cs_which].alignment.x != 0) ||
          (registration.codestreams[cs_which].alignment.y != 0))
        need_creg = true;
      size.x *= registration.codestreams[cs_which].sampling.x;
      size.y *= registration.codestreams[cs_which].sampling.y;
      size += registration.codestreams[cs_which].alignment;
      if ((cs_which == 0) || (size.x < registration.final_layer_size.x))
        registration.final_layer_size.x = size.x;
      if ((cs_which == 0) || (size.y < registration.final_layer_size.y))
        registration.final_layer_size.y = size.y;
    }
  registration.final_layer_size.x =
    ceil_ratio(registration.final_layer_size.x,registration.denominator.x);
  registration.final_layer_size.y =
    ceil_ratio(registration.final_layer_size.y,registration.denominator.y);
  for (cscan=&colour; cscan != NULL; cscan=cscan->next)
    cscan->finalize(&channels);
  finalized = true;
  return need_creg;
}

/*****************************************************************************/
/*                        jx_layer_target::write_jplh                        */
/*****************************************************************************/

jp2_output_box *
  jx_layer_target::write_jplh(bool write_creg_box, int *i_param,
                              void **addr_param)
{
  assert(finalized && !jplh_written);
  if (jplh.exists())
    { // We must have been processing a breakpoint.
      assert(have_breakpoint);
      jplh.close();
      jplh_written = true;
      return NULL;
    }
  owner->open_top_box(&jplh,jp2_compositing_layer_hdr_4cc);
  j2_colour *cscan, *dscan, *default_colour=owner->get_default_colour();
  for (cscan=&colour; cscan != NULL; cscan=cscan->next)
    {
      for (dscan=default_colour; dscan != NULL; dscan=dscan->next)
        if (cscan->compare(dscan))
          break;
      if (dscan == NULL)
        break; // Could not find a match
    }
  if (cscan != NULL)
    { // Default colour spaces not identical to current ones
      jp2_output_box cgrp;
      cgrp.open(&jplh,jp2_colour_group_4cc);
      for (cscan=&colour; cscan != NULL; cscan=cscan->next)
        cscan->save_box(&cgrp);
      cgrp.close();
    }
  if (!owner->get_default_channels()->compare(&channels))
    channels.save_box(&jplh,(id==0));
  if (write_creg_box)
    registration.save_box(&jplh);
  if (!owner->get_default_resolution()->compare(&resolution))
    resolution.save_box(&jplh);
  if (have_breakpoint)
    {
      if (i_param != NULL)
        *i_param = this->i_param;
      if (addr_param != NULL)
        *addr_param = this->addr_param;
      return &jplh;
    }
  jplh.close();
  jplh_written = true;
  return NULL;
}

/*****************************************************************************/
/*                       jx_layer_target::copy_defaults                      */
/*****************************************************************************/

void
  jx_layer_target::copy_defaults(j2_resolution &default_res,
                                 j2_channels &default_channels,
                                 j2_colour &default_colour)
{
  default_res.copy(&resolution);
  if (!channels.needs_opacity_box())
    default_channels.copy(&channels); // Can't write opacity box in jp2 header
  j2_colour *src, *dst;
  for (src=&colour, dst=&default_colour; src != NULL; src=src->next)
    {
      dst->copy(src);
      if (src->next != NULL)
        {
          assert(dst->next == NULL);
          dst = dst->next = new j2_colour;
        }
    }
}

/*****************************************************************************/
/*                  jx_layer_target::adjust_compatibility                    */
/*****************************************************************************/

void
  jx_layer_target::adjust_compatibility(jx_compatibility *compatibility)
{
  assert(finalized);
  if (id > 0)
    compatibility->add_standard_feature(JPX_SF_MULTIPLE_LAYERS);

  if (channels.uses_palette_colour())
    compatibility->add_standard_feature(JPX_SF_PALETTIZED_COLOUR);
  if (channels.has_opacity())
    compatibility->add_standard_feature(JPX_SF_OPACITY_NOT_PREMULTIPLIED);
  if (channels.has_premultiplied_opacity())
    compatibility->add_standard_feature(JPX_SF_OPACITY_PREMULTIPLIED);
  if (channels.needs_opacity_box())
    {
      compatibility->add_standard_feature(JPX_SF_OPACITY_BY_CHROMA_KEY);
      if (id == 0)
        compatibility->set_not_jp2_compatible();
    }

  if (registration.num_codestreams > 1)
    {
      compatibility->add_standard_feature(
                                  JPX_SF_MULTIPLE_CODESTREAMS_PER_LAYER);
      if (id == 0)
        {
          compatibility->set_not_jp2_compatible();
          compatibility->set_not_jpxb_compatible();
        }
      for (int n=1; n < registration.num_codestreams; n++)
        if (registration.codestreams[n].sampling !=
            registration.codestreams[0].sampling)
          {
            compatibility->add_standard_feature(JPX_SF_SCALING_WITHIN_LAYER);
            break;
          }
    }

  jp2_resolution res(&resolution);
  float aspect = res.get_aspect_ratio();
  if ((aspect < 0.99F) || (aspect > 1.01F))
    compatibility->add_standard_feature(JPX_SF_SAMPLES_NOT_SQUARE);

  bool have_jp2_compatible_space = false;
  bool have_non_vendor_space = false;
  int best_precedence = -128;
  j2_colour *cbest=NULL, *cscan;
  for (cscan=&colour; cscan != NULL; cscan=cscan->next)
    {
      jp2_colour clr(cscan);
      if (clr.get_precedence() > best_precedence)
        {
          best_precedence = clr.get_precedence();
          cbest = cscan;
        }
    }
  for (cscan=&colour; cscan != NULL; cscan=cscan->next)
    { // Add feature flags to represent each colour description.
      if (cscan->is_jp2_compatible())
        have_jp2_compatible_space = true;
      jp2_colour clr(cscan);
      jp2_colour_space space = clr.get_space();
      bool add_to_both = (cscan==cbest);
      if (space == JP2_bilevel1_SPACE)
        compatibility->add_standard_feature(JPX_SF_BILEVEL1,add_to_both);
      else if (space == JP2_YCbCr1_SPACE)
        compatibility->add_standard_feature(JPX_SF_YCbCr1,add_to_both);
      else if (space == JP2_YCbCr2_SPACE)
        compatibility->add_standard_feature(JPX_SF_YCbCr2,add_to_both);
      else if (space == JP2_YCbCr3_SPACE)
        compatibility->add_standard_feature(JPX_SF_YCbCr3,add_to_both);
      else if (space == JP2_PhotoYCC_SPACE)
        compatibility->add_standard_feature(JPX_SF_PhotoYCC,add_to_both);
      else if (space == JP2_CMY_SPACE)
        compatibility->add_standard_feature(JPX_SF_CMY,add_to_both);
      else if (space == JP2_CMYK_SPACE)
        compatibility->add_standard_feature(JPX_SF_CMYK,add_to_both);
      else if (space == JP2_YCCK_SPACE)
        compatibility->add_standard_feature(JPX_SF_YCCK,add_to_both);
      else if (space == JP2_CIELab_SPACE)
        {
          if (clr.check_cie_default())
            compatibility->add_standard_feature(JPX_SF_LAB_DEFAULT,
                                                add_to_both);
          else
            compatibility->add_standard_feature(JPX_SF_LAB,add_to_both);
        }
      else if (space == JP2_bilevel2_SPACE)
        compatibility->add_standard_feature(JPX_SF_BILEVEL2,add_to_both);
      else if (space == JP2_sRGB_SPACE)
        compatibility->add_standard_feature(JPX_SF_sRGB,add_to_both);
      else if (space == JP2_sLUM_SPACE)
        compatibility->add_standard_feature(JPX_SF_sLUM,add_to_both);
      else if (space == JP2_sYCC_SPACE)
        compatibility->add_standard_feature(JPX_SF_sYCC,add_to_both);
      else if (space == JP2_CIEJab_SPACE)
        {
          if (clr.check_cie_default())
            compatibility->add_standard_feature(JPX_SF_JAB_DEFAULT,
                                                add_to_both);
          else
            compatibility->add_standard_feature(JPX_SF_JAB,add_to_both);
        }
      else if (space == JP2_esRGB_SPACE)
        compatibility->add_standard_feature(JPX_SF_esRGB,add_to_both);
      else if (space == JP2_ROMMRGB_SPACE)
        compatibility->add_standard_feature(JPX_SF_ROMMRGB,add_to_both);
      else if (space == JP2_YPbPr60_SPACE)
        {} // Need a compatibility code for this -- none defined originally
      else if (space == JP2_YPbPr50_SPACE)
        {} // Need a compatibility code for this -- none defined originally
      else if (space == JP2_esYCC_SPACE)
        {} // Need a compatibility code for this -- none defined originally
      else if (space == JP2_iccLUM_SPACE)
        compatibility->add_standard_feature(JPX_SF_RESTRICTED_ICC,add_to_both);
      else if (space == JP2_iccRGB_SPACE)
        compatibility->add_standard_feature(JPX_SF_RESTRICTED_ICC,add_to_both);
      else if (space == JP2_iccANY_SPACE)
        compatibility->add_standard_feature(JPX_SF_ANY_ICC,add_to_both);
      else if (space == JP2_vendor_SPACE)
        {} // Need a compatibility code for this -- none defined originally
      if (space != JP2_vendor_SPACE)
        have_non_vendor_space = true;
    }
  if ((id == 0) && !have_jp2_compatible_space)
    compatibility->set_not_jp2_compatible();
  if ((id == 0) && !have_non_vendor_space)
    compatibility->set_not_jpxb_compatible();
}

/*****************************************************************************/
/*                     jx_layer_target::uses_codestream                      */
/*****************************************************************************/

bool
  jx_layer_target::uses_codestream(int idx)
{
  for (int n=0; n < registration.num_codestreams; n++)
    if (registration.codestreams[n].codestream_id == idx)
      return true;
  return false;
}


/* ========================================================================= */
/*                            jpx_layer_target                               */
/* ========================================================================= */

/*****************************************************************************/
/*                    jpx_layer_target::access_channels                      */
/*****************************************************************************/

jp2_channels
  jpx_layer_target::access_channels()
{
  assert(state != NULL);
  return jp2_channels(&state->channels);
}

/*****************************************************************************/
/*                   jpx_layer_target::access_resolution                     */
/*****************************************************************************/

jp2_resolution
  jpx_layer_target::access_resolution()
{
  assert(state != NULL);
  return jp2_resolution(&state->resolution);
}

/*****************************************************************************/
/*                       jpx_layer_target::add_colour                        */
/*****************************************************************************/

jp2_colour
  jpx_layer_target::add_colour(int precedence, kdu_byte approx)
{
  assert(state != NULL);
  if ((precedence < -128) || (precedence > 127) || (approx > 4))
    { KDU_ERROR_DEV(e,61); e <<
        KDU_TXT("Invalid `precedence' or `approx' parameter supplied "
        "to `jpx_layer_target::add_colour'.  Legal values for the precedence "
        "parameter must lie in the range -128 to +127, while legal values "
        "for the approximation level parameter are 0, 1, 2, 3 and 4.");
    }
  if (state->last_colour == NULL)
    state->last_colour = &state->colour;
  else
    state->last_colour = state->last_colour->next = new j2_colour;
  state->last_colour->precedence = precedence;
  state->last_colour->approx = approx;
  return jp2_colour(state->last_colour);
}

/*****************************************************************************/
/*                     jpx_layer_target::access_colour                       */
/*****************************************************************************/

jp2_colour
  jpx_layer_target::access_colour(int which)
{
  assert(state != NULL);
  j2_colour *scan = NULL;
  if (which >= 0)
    for (scan=&state->colour; (scan != NULL) && (which > 0); which--)
      scan = scan->next;
  return jp2_colour(scan);
}

/*****************************************************************************/
/*               jpx_layer_target::set_codestream_registration               */
/*****************************************************************************/

void
  jpx_layer_target::set_codestream_registration(int codestream_id,
                                                kdu_coords alignment,
                                                kdu_coords sampling,
                                                kdu_coords denominator)
{
  int n;
  assert(state != NULL);
  jx_registration *reg = &state->registration;
  jx_registration::jx_layer_stream *str;
  if (reg->num_codestreams == 0)
    reg->denominator = denominator;
  else if (reg->denominator != denominator)
    { KDU_ERROR_DEV(e,62); e <<
        KDU_TXT("The denominator values supplied via all calls to "
        "`jpx_layer_target::set_codestream_registration' within the same "
        "compositing layer must be identical.  This is because the "
        "codestream registration (creg) box can record only one denominator "
        "(point density) to be shared by all the codestream sampling and "
        "alignment parameters.");
    }
  if ((denominator.x < 1) || (denominator.x >= (1<<16)) ||
      (denominator.y < 1) || (denominator.y >= (1<<16)) ||
      (alignment.x < 0) || (alignment.y < 0) ||
      (alignment.x > 255) || (alignment.y > 255) ||
      (alignment.x >= denominator.x) || (alignment.y >= denominator.y) ||
      (sampling.x < 1) || (sampling.y < 1) ||
      (sampling.x > 255) || (sampling.y > 255))
    { KDU_ERROR_DEV(e,63); e <<
        KDU_TXT("Illegal alignment or sampling parameters passed to "
        "`jpx_layer_target::set_codestream_registration'.  The alignment "
        "offset and sampling numerator values must be non-negative (non-zero "
        "for sampling factors) and no larger than 255; moreover, the "
        "alignment offsets must be strictly less than the denominator "
        "(point density) values and the common sampling denominator must "
        "lie be in the range 1 to 65535.");
    }

  for (n=0; n < reg->num_codestreams; n++)
    {
      str = reg->codestreams + n;
      if (str->codestream_id == codestream_id)
        break;
    }
  if (n == reg->num_codestreams)
    { // Add new element
      if (reg->max_codestreams == reg->num_codestreams)
        { // Augment array
          int new_max_cs = reg->max_codestreams + n + 2;
          str = new jx_registration::jx_layer_stream[new_max_cs];
          for (n=0; n < reg->num_codestreams; n++)
            str[n] = reg->codestreams[n];
          if (reg->codestreams != NULL)
            delete[] reg->codestreams;
          reg->codestreams = str;
          reg->max_codestreams = new_max_cs;
        }
      reg->num_codestreams++;
      str = reg->codestreams + n;
    }
  str->codestream_id = codestream_id;
  str->sampling = sampling;
  str->alignment = alignment;
}

/*****************************************************************************/
/*                      jpx_layer_target::set_breakpoint                     */
/*****************************************************************************/

void
  jpx_layer_target::set_breakpoint(int i_param, void *addr_param)
{
  assert(state != NULL);
  state->have_breakpoint = true;
  state->i_param = i_param;
  state->addr_param = addr_param;
}


/* ========================================================================= */
/*                                jx_target                                  */
/* ========================================================================= */

/*****************************************************************************/
/*                           jx_target::jx_target                            */
/*****************************************************************************/

jx_target::jx_target(jp2_family_tgt *tgt)
{
  this->ultimate_tgt = tgt;
  this->simulated_tgt = NULL;
  last_opened_top_box = NULL;
  last_codestream_threshold = 0;
  need_creg_boxes = false;
  headers_in_progress = main_header_complete = false;
  num_codestreams = 0;
  num_layers = 0;
  codestreams = NULL;
  last_codestream = NULL;
  layers = NULL;
  last_layer = NULL;
  meta_manager.tree = new jx_metanode(&meta_manager);
}

/*****************************************************************************/
/*                          jx_target::~jx_target                            */
/*****************************************************************************/

jx_target::~jx_target()
{
  jx_codestream_target *cp;
  jx_layer_target *lp;
  j2_colour *cscan;

  while ((cp=codestreams) != NULL)
    {
      codestreams = cp->next;
      delete cp;
    }
  while ((lp=layers) != NULL)
    {
      layers = lp->next;
      delete lp;
    }
  while ((cscan=default_colour.next) != NULL)
    { default_colour.next = cscan->next; delete cscan; }
  if (simulated_tgt != NULL)
    delete simulated_tgt;
}

/*****************************************************************************/
/*                         jx_target::open_top_box                           */
/*****************************************************************************/

kdu_long
  jx_target::open_top_box(jp2_output_box *box, kdu_uint32 box_type,
                          bool simulate_write)
{
  kdu_long file_pos = ultimate_tgt->get_bytes_written();
  if (simulated_tgt != NULL)
    file_pos = simulated_tgt->get_bytes_written();
  if (box == NULL)
    { last_opened_top_box = NULL; return file_pos; }
  if ((last_opened_top_box != NULL) && (last_opened_top_box->exists()))
    { KDU_ERROR_DEV(e,64); e <<
        KDU_TXT("Attempting to open a new top-level box within "
        "a JPX file, while another top-level box is already open!  "
        "Problem may be caused by failing to complete a code-stream and "
        "close its box before attempting to write a second code-stream.");
    }
  last_opened_top_box = NULL;
  if (simulate_write)
    {
      if (simulated_tgt == NULL)
        {
          simulated_tgt = new jp2_family_tgt;
          simulated_tgt->open(ultimate_tgt->get_bytes_written());
        }
      box->open(simulated_tgt,box_type);
    }
  else
    {
      if (simulated_tgt != NULL)
        {
          delete simulated_tgt;
          simulated_tgt = NULL;
        }
      box->open(ultimate_tgt,box_type);
    }
  last_opened_top_box = box;
  return file_pos;
}


/* ========================================================================= */
/*                                 jpx_target                                */
/* ========================================================================= */

/*****************************************************************************/
/*                           jpx_target::jpx_target                          */
/*****************************************************************************/

jpx_target::jpx_target()
{
  state = NULL;
}

/*****************************************************************************/
/*                              jpx_target::open                             */
/*****************************************************************************/

void
  jpx_target::open(jp2_family_tgt *tgt)
{
  if (state != NULL)
    { KDU_ERROR_DEV(e,65); e <<
        KDU_TXT("Attempting to open a `jpx_target' object which "
        "is already opened for writing a JPX file.");
    }
  state = new jx_target(tgt);
}

/*****************************************************************************/
/*                             jpx_target::close                             */
/*****************************************************************************/
bool
  jpx_target::close()
{
  if (state == NULL)
    return false;
  jx_codestream_target *cp;
  for (cp=state->codestreams; cp != NULL; cp=cp->next)
    if (!cp->is_complete())
      break;
  
  if (state->main_header_complete && (cp != NULL))
    { KDU_WARNING_DEV(w,2); w <<
        KDU_TXT("Started writing a JPX file, but failed to "
        "write all codestreams before calling `jpx_target::close'.");
    }
  else if (state->headers_in_progress)
    { KDU_WARNING_DEV(w,3); w <<
        KDU_TXT("Started writing JPX file headers, but failed "
        "to finish initiated sequence of calls to "
        "`jpx_target::write_headers'.  Problem is most likely due to "
        "the use of `jpx_codestream_target::set_breakpoint' or "
        "`jpx_layer_target::set_breakpoint' and failure to handle the "
        "breakpoints via multiple calls to `jpx_target::write_headers'.");
    }
  else if (state->main_header_complete)
    { // Write any outstanding headers
      bool missed_breakpoints = false;
      while (write_headers() != NULL)
        missed_breakpoints = true;
      if (missed_breakpoints)
        { KDU_WARNING_DEV(w,4); w <<
            KDU_TXT("Failed to catch all breakpoints installed "
            "via `jpx_codestream_target::set_breakpoint' or "
            "`jpx_layer_target::set_breakpoint'.  All required compositing "
            "layer header boxes and codestream header boxes have been "
            "automatically written while closing the file, but some of these "
            "included application-installed breakpoints where the application "
            "would ordinarily have written its own extra boxes.  This "
            "suggests that the application has failed to make sufficient "
            "explicit calls to `jpx_target::write_headers'.");
        }
    }

  if (access_data_references().get_num_urls() > 0)
    {
      jp2_output_box dtbl; state->open_top_box(&dtbl,jp2_dtbl_4cc);
      state->data_references.save_box(&dtbl);
    }

  delete state;
  state = NULL;
  return true;
}

/*****************************************************************************/
/*                     jpx_target::access_data_references                    */
/*****************************************************************************/

jp2_data_references
  jpx_target::access_data_references()
{
  return jp2_data_references(&state->data_references);
}

/*****************************************************************************/
/*                      jpx_target::access_compatibility                     */
/*****************************************************************************/

jpx_compatibility
  jpx_target::access_compatibility()
{
  jpx_compatibility result;
  if (state != NULL)
    result = jpx_compatibility(&state->compatibility);
  return result;
}

/*****************************************************************************/
/*                          jpx_target::add_codestream                       */
/*****************************************************************************/

jpx_codestream_target
  jpx_target::add_codestream()
{
  jpx_codestream_target result;
  if (state != NULL)
    {
      assert(!(state->main_header_complete || state->headers_in_progress));
      jx_codestream_target *cs =
        new jx_codestream_target(state,state->num_codestreams);
      if (state->last_codestream == NULL)
        state->codestreams = state->last_codestream = cs;
      else
        state->last_codestream = state->last_codestream->next = cs;
      state->num_codestreams++;
      result = jpx_codestream_target(cs);
    }
  return result;
}

/*****************************************************************************/
/*                            jpx_target::add_layer                          */
/*****************************************************************************/

jpx_layer_target
  jpx_target::add_layer()
{
  jpx_layer_target result;
  if (state != NULL)
    {
      assert(!(state->main_header_complete || state->headers_in_progress));
      jx_layer_target *layer =
        new jx_layer_target(state,state->num_layers);
      if (state->last_layer == NULL)
        state->layers = state->last_layer = layer;
      else
        state->last_layer = state->last_layer->next = layer;
      state->num_layers++;
      result = jpx_layer_target(layer);
    }
  return result;
}

/*****************************************************************************/
/*                       jpx_target::access_composition                      */
/*****************************************************************************/

jpx_composition
  jpx_target::access_composition()
{
  jpx_composition result;
  if (state != NULL)
    result = jpx_composition(&state->composition);
  return result;
}

/*****************************************************************************/
/*                      jpx_target::access_meta_manager                      */
/*****************************************************************************/

jpx_meta_manager
  jpx_target::access_meta_manager()
{
  jpx_meta_manager result;
  if (state != NULL)
    result = jpx_meta_manager(&state->meta_manager);
  return result;
}

/*****************************************************************************/
/*                        jpx_target::write_headers                          */
/*****************************************************************************/

jp2_output_box *
  jpx_target::write_headers(int *i_param, void **addr_param,
                            int codestream_threshold)
{
  assert(state != NULL);
  if ((state->num_codestreams == 0) || (state->num_layers == 0))
    { KDU_ERROR_DEV(e,66); e <<
        KDU_TXT("Attempting to write a JPX file which has no "
        "code-stream, or no compositing layer.");
    }

 
  jx_codestream_target *cp;
  jx_layer_target *lp;

  if (!(state->main_header_complete || state->headers_in_progress))
    {
      // Start by finalizing everything
      state->need_creg_boxes = false;
      for (cp=state->codestreams; cp != NULL; cp=cp->next)
        cp->finalize();
      for (lp=state->layers; lp != NULL; lp=lp->next)
        if (lp->finalize())
          state->need_creg_boxes = true;
      state->composition.finalize(state);

      // Next, initialize the defaults
      state->codestreams->copy_defaults(state->default_dimensions,
                                        state->default_palette,
                                        state->default_component_map);
      state->layers->copy_defaults(state->default_resolution,
                                   state->default_channels,
                                   state->default_colour);

      // Now add compatibility information
      for (cp=state->codestreams; cp != NULL; cp=cp->next)
        cp->adjust_compatibility(&state->compatibility);
      for (lp=state->layers; lp != NULL; lp=lp->next)
        lp->adjust_compatibility(&state->compatibility);
      state->composition.adjust_compatibility(&state->compatibility,state);

      // Now write the initial header boxes
      state->open_top_box(&state->box,jp2_signature_4cc);
      state->box.write(jp2_signature);
      state->box.close();

      state->compatibility.save_boxes(state);
      state->composition.save_box(state);
      state->open_top_box(&state->box,jp2_header_4cc);
      state->default_dimensions.save_boxes(&state->box);
      j2_colour *compat, *cscan;
      for (compat=&state->default_colour; compat != NULL; compat=compat->next)
        if (compat->is_jp2_compatible())
          break;
      if (compat != NULL)
        compat->save_box(&state->box); // Try save JP2 compatible colour first
      for (cscan=&state->default_colour; cscan != NULL; cscan=cscan->next)
        if (cscan != compat)
          cscan->save_box(&state->box);
      state->default_palette.save_box(&state->box);
      state->default_component_map.save_box(&state->box);
      state->default_channels.save_box(&state->box,true);
      state->default_resolution.save_box(&state->box);
      state->box.close();
      state->main_header_complete = true;
    }

  // In case the application accidentally changes the threshold value between
  // incomplete calls to this function, we remember it here.  This ensures
  // that we take up at the correct place again after an application-installed
  // breakpoint
  if (state->headers_in_progress)
    codestream_threshold = state->last_codestream_threshold;
  state->last_codestream_threshold = codestream_threshold;

  state->headers_in_progress = true;

  int idx;
  jp2_output_box *bpt_box=NULL;
  for (idx=0, cp=state->codestreams; cp != NULL; cp=cp->next, idx++)
    {
      if ((codestream_threshold >= 0) && (idx > codestream_threshold))
        break; // We have written enough for now
      if ((!cp->is_chdr_complete()) &&
          ((bpt_box = cp->write_chdr(i_param,addr_param)) != NULL))
        return bpt_box;
    }
  for (lp=state->layers; lp != NULL; lp=lp->next)
    {
      if ((!lp->is_jplh_complete()) &&
          ((bpt_box = lp->write_jplh(state->need_creg_boxes,
                                     i_param,addr_param)) != NULL))
        return bpt_box;
      if ((codestream_threshold >= 0) &&
          lp->uses_codestream(codestream_threshold))
        break;
    }

  state->headers_in_progress = false;
  return NULL;
}

/*****************************************************************************/
/*                        jpx_target::write_metadata                         */
/*****************************************************************************/

jp2_output_box *
  jpx_target::write_metadata(int *i_param, void **addr_param)
{
  if (state->meta_manager.active_metagroup == NULL)
    { // Calling this function for the first time
      state->meta_manager.simulate_write = state->meta_manager.have_link_nodes;
      state->meta_manager.tree->clear_write_state(true);
      if (state->meta_manager.target != NULL)
        { KDU_ERROR_DEV(e,67); e <<
            KDU_TXT("Trying to invoke `jpx_target::write_metadata' "
            "after all metadata has already been written to the file.");
        }
      state->meta_manager.target = state;
    }
  jp2_output_box *result;
  if ((result=state->meta_manager.write_metadata(i_param,addr_param)) != NULL)
    return result;
  if (state->meta_manager.simulate_write)
    { // Finished the simulation phase
      state->meta_manager.simulate_write = false;
      state->meta_manager.tree->clear_write_state(false);
      result = state->meta_manager.write_metadata(i_param,addr_param);
    }
  return result;
}
